#!/usr/bin/env python3
"""
Madagascar (2005) PC - Phase 1: Live Memory & Offset Validator (Refactored)
==========================================================================
Basiert auf den Erkenntnissen aus 'game_memory.py' (MaxStache).

Pointer-Kette (relativ zur dynamischen Module-Base von 'Game.exe'):
  mod_base = BaseAddress("Game.exe")
  Level 1  = [mod_base + 0x0021818C]
  Level 2  = [Level 1  + 0x000000A8]
  Player   = [Level 2  + 0x00000230]

Verifizierte Offsets der Spielfigur:
  X-Koordinate: [Player] + 0x150 (Primary) / + 0x1F4 (Secondary)
  Y-Koordinate: [Player] + 0x154 (Primary) / + 0x1F8 (Secondary)
  Z-Koordinate: [Player] + 0x158 (Primary) / + 0x1FC (Secondary)

Zusätzliche Engine-Adressen:
  Pause-Status:      mod_base + 0x0022A520 (uint32: 0 = Running, !=0 = Paused)
  Kamera Pitch:      mod_base + 0x002181FC (float)
  Kamera Yaw:        mod_base + 0x00218220 (float)
  Physik-Schreibbefehl: mod_base + 0x00028E9C (6 Bytes: D9 9D F8 01 00 00 -> fstp dword ptr [ebp+1F8h])
"""

import os
import sys
import time
import struct
import ctypes
from ctypes import wintypes

# ==============================================================================
# 1. AUTO-ADMINISTRATOR ELEVATION
# ==============================================================================
def ensure_admin_privileges():
    """Prüft Admin-Rechte und startet sich via UAC-Prompt ('runas') neu falls nötig."""
    try:
        is_admin = ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        is_admin = False

    if not is_admin:
        print("[UAC] Keine Administrator-Rechte erkannt. Starte UAC-Prompt...")
        script_path = os.path.abspath(__file__)
        params = f'"{script_path}"'
        if len(sys.argv) > 1:
            params += ' ' + ' '.join(f'"{arg}"' for arg in sys.argv[1:])

        # ShellExecuteW mit Verb 'runas' erzwingt Windows UAC Dialog
        ret = ctypes.windll.shell32.ShellExecuteW(
            None,
            "runas",
            sys.executable,
            params,
            None,
            1  # SW_SHOWNORMAL
        )
        if ret > 32:
            # Erfolgreich an erhöhten Prozess übergeben -> aktuellen Prozess beenden
            sys.exit(0)
        else:
            print(f"[-] UAC-Abfrage abgelehnt oder fehlgeschlagen (Fehlercode: {ret}).")
            print("    Das Skript läuft ohne Administrator-Rechte weiter (kann zu Fehlern führen).\n")

ensure_admin_privileges()

# ==============================================================================
# 2. WIN32 API DEFINITIONEN & PROCESS MEMORY
# ==============================================================================
kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
user32   = ctypes.WinDLL('user32', use_last_error=True)
psapi    = ctypes.WinDLL('psapi', use_last_error=True)

PROCESS_ALL_ACCESS     = 0x1F0FFF
PAGE_EXECUTE_READWRITE = 0x40
TH32CS_SNAPPROCESS     = 0x00000002
LIST_MODULES_ALL       = 0x03

class PROCESSENTRY32(ctypes.Structure):
    _fields_ = [
        ('dwSize', wintypes.DWORD),
        ('cntUsage', wintypes.DWORD),
        ('th32ProcessID', wintypes.DWORD),
        ('th32DefaultHeapID', ctypes.c_void_p),
        ('th32ModuleID', wintypes.DWORD),
        ('cntThreads', wintypes.DWORD),
        ('th32ParentProcessID', wintypes.DWORD),
        ('pcPriClassBase', wintypes.LONG),
        ('dwFlags', wintypes.DWORD),
        ('szExeFile', ctypes.c_char * wintypes.MAX_PATH)
    ]

def find_process_id(process_name: str) -> int:
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if snap == wintypes.HANDLE(-1).value:
        return 0
    pe = PROCESSENTRY32()
    pe.dwSize = ctypes.sizeof(PROCESSENTRY32)
    found_pid = 0
    if kernel32.Process32First(snap, ctypes.byref(pe)):
        while True:
            if pe.szExeFile.decode('utf-8', errors='ignore').lower() == process_name.lower():
                found_pid = pe.th32ProcessID
                break
            if not kernel32.Process32Next(snap, ctypes.byref(pe)):
                break
    kernel32.CloseHandle(snap)
    return found_pid

class ProcessMemory:
    def __init__(self, pid: int):
        self.pid = pid
        self.handle = kernel32.OpenProcess(PROCESS_ALL_ACCESS, False, pid)
        if not self.handle:
            raise RuntimeError(f"OpenProcess fehlgeschlagen! Error: {ctypes.get_last_error()}")
        self.module_base = self._resolve_module_base("Game.exe")

    def _resolve_module_base(self, target_module: str) -> int:
        """Ermittelt die dynamische Basisadresse von Game.exe via psapi.EnumProcessModulesEx."""
        h_mods = (wintypes.HMODULE * 1024)()
        cb_needed = wintypes.DWORD()
        target_lower = target_module.lower()

        if psapi.EnumProcessModulesEx(self.handle, ctypes.byref(h_mods), ctypes.sizeof(h_mods), ctypes.byref(cb_needed), LIST_MODULES_ALL):
            count = cb_needed.value // ctypes.sizeof(wintypes.HMODULE)
            for i in range(count):
                mod = h_mods[i]
                mod_name = ctypes.create_string_buffer(wintypes.MAX_PATH)
                if psapi.GetModuleBaseNameA(self.handle, mod, mod_name, wintypes.MAX_PATH):
                    name_str = mod_name.value.decode('utf-8', errors='ignore').lower()
                    if name_str == target_lower:
                        return ctypes.cast(mod, ctypes.c_void_p).value or 0

            # Fallback: Das allererste geladene Modul ist bei PE-Prozessen die Main-Executable
            if count > 0:
                first_base = ctypes.cast(h_mods[0], ctypes.c_void_p).value or 0
                if first_base:
                    return first_base

        # Standard-Fallback für 32-bit Win32 PE
        print("[!] Warnung: ModuleBase nicht über EnumProcessModules gefunden. Nutze Fallback 0x00400000.")
        return 0x00400000

    def close(self):
        if self.handle:
            kernel32.CloseHandle(self.handle)
            self.handle = None

    def read_bytes(self, address: int, size: int) -> bytes:
        buf = ctypes.create_string_buffer(size)
        bytes_read = ctypes.c_size_t()
        if not kernel32.ReadProcessMemory(self.handle, ctypes.c_void_p(address), buf, size, ctypes.byref(bytes_read)):
            return b""
        return buf.raw[:bytes_read.value]

    def write_bytes(self, address: int, data: bytes) -> bool:
        old_protect = wintypes.DWORD()
        if not kernel32.VirtualProtectEx(self.handle, ctypes.c_void_p(address), len(data), PAGE_EXECUTE_READWRITE, ctypes.byref(old_protect)):
            return False
        bytes_written = ctypes.c_size_t()
        success = kernel32.WriteProcessMemory(self.handle, ctypes.c_void_p(address), data, len(data), ctypes.byref(bytes_written))
        kernel32.VirtualProtectEx(self.handle, ctypes.c_void_p(address), len(data), old_protect, ctypes.byref(old_protect))
        return bool(success and bytes_written.value == len(data))

    def read_uint32(self, address: int) -> int:
        raw = self.read_bytes(address, 4)
        if len(raw) == 4:
            return struct.unpack('<I', raw)[0]
        return 0

    def read_float(self, address: int) -> float:
        raw = self.read_bytes(address, 4)
        if len(raw) == 4:
            return struct.unpack('<f', raw)[0]
        return 0.0

    def write_float(self, address: int, val: float) -> bool:
        return self.write_bytes(address, struct.pack('<f', val))

# ==============================================================================
# 3. DYNAMISCHE POINTER-KETTEN-AUFLÖSUNG (game_memory.py Architektur)
# ==============================================================================
# Relative Offsets bezogen auf die Base-Adresse von Game.exe:
OFFSET_BASE_PTR      = 0x0021818C
OFFSET_LEVEL_2       = 0x000000A8
OFFSET_LEVEL_3       = 0x00000230

# Offsets für Spielerkoordinaten in der Player-Instanz:
OFF_X_PRIMARY        = 0x150
OFF_Y_PRIMARY        = 0x154
OFF_Z_PRIMARY        = 0x158

OFF_X_SECONDARY      = 0x1F4
OFF_Y_SECONDARY      = 0x1F8
OFF_Z_SECONDARY      = 0x1FC

# Statische Engine-Offsets (relativ zu Game.exe Base):
OFFSET_PAUSED        = 0x0022A520
OFFSET_CAM_PITCH     = 0x002181FC
OFFSET_CAM_YAW       = 0x00218220
OFFSET_PHYSICS_OPCODE= 0x00028E9C  # 0x00428E9C - 0x00400000 = 0x00028E9C

VK_T = 0x54  # Teleportieren (+10 Y)
VK_P = 0x50  # Physik-Patch umschalten
VK_Q = 0x51  # Beenden

def is_key_pressed(vk: int) -> bool:
    return bool(user32.GetAsyncKeyState(vk) & 0x8000)

def resolve_player_entity(mem: ProcessMemory) -> tuple[int, str]:
    """
    Löst die vollständige 3-Level-Pointer-Chain auf:
      base -> [base + 0x21818C] -> [+0xA8] -> [+0x230]
    Gibt die finale Player-Entity-Adresse und einen Statusstring zurück.
    """
    mod_base = mem.module_base
    lvl1 = mem.read_uint32(mod_base + OFFSET_BASE_PTR)
    if not (0x00400000 <= lvl1 <= 0x7FFE0000):
        return 0, f"Lvl1 ungültig (0x{lvl1:08X})"

    lvl2 = mem.read_uint32(lvl1 + OFFSET_LEVEL_2)
    if not (0x00400000 <= lvl2 <= 0x7FFE0000):
        return 0, f"Lvl2 ungültig (0x{lvl2:08X})"

    player_entity = mem.read_uint32(lvl2 + OFFSET_LEVEL_3)
    if not (0x00400000 <= player_entity <= 0x7FFE0000):
        return 0, f"Player ungültig (0x{player_entity:08X})"

    return player_entity, "OK"

# ==============================================================================
# 4. MAIN INTERACTION LOOP
# ==============================================================================
def main():
    print("==================================================================")
    print(" MADAGASCAR (2005) - REFACTORED MEMORY & OFFSET VALIDATOR")
    print(" (Pointer-Chain: [Base+0x21818C] -> [+0xA8] -> [+0x230])")
    print("==================================================================")

    print("[1] Suche nach Prozess 'Game.exe'...")
    pid = find_process_id("Game.exe")
    while not pid:
        print("    Warte auf 'Game.exe'... Bitte starte das Spiel!", end='\r')
        time.sleep(1.5)
        pid = find_process_id("Game.exe")

    print(f"\n[+] 'Game.exe' gefunden! Process ID (PID): {pid}")

    try:
        mem = ProcessMemory(pid)
    except Exception as e:
        print(f"[-] Fehler beim Zugriff auf den Prozess: {e}")
        return

    mod_base = mem.module_base
    physics_addr = mod_base + OFFSET_PHYSICS_OPCODE
    cam_pitch_addr = mod_base + OFFSET_CAM_PITCH
    cam_yaw_addr = mod_base + OFFSET_CAM_YAW
    paused_addr = mod_base + OFFSET_PAUSED

    print(f"[+] Module Base: 0x{mod_base:08X}")
    print(f"[+] Physics Opcode Addr: 0x{physics_addr:08X}")

    original_physics_bytes = mem.read_bytes(physics_addr, 6)
    if not original_physics_bytes:
        original_physics_bytes = b"\xD9\x9D\xF8\x01\x00\x00"
    print(f"[+] Gesicherte Physik-Instruktion: {original_physics_bytes.hex().upper()}")

    physics_patched = False
    last_key_t = False
    last_key_p = False

    print("\n--- STEUERUNG ---")
    print(" [T] : Y-Höhenkoordinate um +10.0 Einheiten nach oben setzen")
    print(" [P] : Physik-Overwrite Patch (NOPs <-> Original) umschalten")
    print(" [Q] : Beenden (stellt Physik-Instruktion wieder her)")
    print("------------------------------------------------------------------\n")

    try:
        while True:
            player_addr, status_str = resolve_player_entity(mem)

            cam_pitch = mem.read_float(cam_pitch_addr)
            cam_yaw   = mem.read_float(cam_yaw_addr)
            is_paused = mem.read_uint32(paused_addr) != 0

            if player_addr != 0:
                # Koordinaten aus beiden bekannten Offset-Paaren lesen:
                x1 = mem.read_float(player_addr + OFF_X_PRIMARY)
                y1 = mem.read_float(player_addr + OFF_Y_PRIMARY)
                z1 = mem.read_float(player_addr + OFF_Z_PRIMARY)

                x2 = mem.read_float(player_addr + OFF_X_SECONDARY)
                y2 = mem.read_float(player_addr + OFF_Y_SECONDARY)
                z2 = mem.read_float(player_addr + OFF_Z_SECONDARY)

                patch_str = "[NOP-PATCH]" if physics_patched else "[ORIGINAL]"
                pause_str = "[PAUSE]" if is_paused else "[RUN]"

                output = (
                    f"\r{pause_str} {patch_str} Entity: 0x{player_addr:08X} | "
                    f"Pos(+150): ({x1:8.2f}, {y1:8.2f}, {z1:8.2f}) | "
                    f"Pos(+1F4): ({x2:8.2f}, {y2:8.2f}, {z2:8.2f}) | "
                    f"Yaw: {cam_yaw:5.2f}"
                )
                sys.stdout.write(output)
                sys.stdout.flush()

                # Hotkey: [T] Teleportieren
                key_t_now = is_key_pressed(VK_T)
                if key_t_now and not last_key_t:
                    # In beide Offset-Varianten schreiben, um vollständige Synchronisation zu gewährleisten
                    new_y1 = y1 + 10.0
                    new_y2 = y2 + 10.0
                    mem.write_float(player_addr + OFF_Y_PRIMARY, new_y1)
                    mem.write_float(player_addr + OFF_Y_SECONDARY, new_y2)
                    sys.stdout.write(f"\n[ACTION] Y-Koordinate erhöht: {y1:.2f} -> {new_y1:.2f}\n")
                    sys.stdout.flush()
                last_key_t = key_t_now

            else:
                sys.stdout.write(f"\r[STATUS: WARTEN] Player Pointer nicht bereit ({status_str}). Hauptmenü oder Ladebildschirm?        ")
                sys.stdout.flush()

            # Hotkey: [P] Toggle Physics Patch
            key_p_now = is_key_pressed(VK_P)
            if key_p_now and not last_key_p:
                if not physics_patched:
                    nop_bytes = b"\x90" * len(original_physics_bytes)
                    if mem.write_bytes(physics_addr, nop_bytes):
                        physics_patched = True
                        sys.stdout.write(f"\n[ACTION] Physik-Overwrite 0x{physics_addr:08X} mit NOPs (0x90) überschrieben!\n")
                    else:
                        sys.stdout.write(f"\n[FEHLER] Konnte NOPs bei 0x{physics_addr:08X} nicht schreiben!\n")
                else:
                    if mem.write_bytes(physics_addr, original_physics_bytes):
                        physics_patched = False
                        sys.stdout.write(f"\n[ACTION] Physik-Instruktion 0x{physics_addr:08X} wiederhergestellt!\n")
                    else:
                        sys.stdout.write(f"\n[FEHLER] Konnte Original-Bytes bei 0x{physics_addr:08X} nicht wiederherstellen!\n")
                sys.stdout.flush()
            last_key_p = key_p_now

            # Hotkey: [Q] Beenden
            if is_key_pressed(VK_Q):
                print("\n[INFO] Beende Programm...")
                break

            time.sleep(0.05)

    except KeyboardInterrupt:
        print("\n[INFO] Abbruch durch Benutzer.")

    finally:
        if physics_patched:
            print("[CLEANUP] Stelle originale Physik-Instruktion wieder her...")
            mem.write_bytes(physics_addr, original_physics_bytes)
        mem.close()
        print("[INFO] Handle geschlossen. Fertig.")

if __name__ == "__main__":
    main()
