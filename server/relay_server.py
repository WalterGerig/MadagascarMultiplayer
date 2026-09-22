#!/usr/bin/env python3
"""
Madagascar (2005) Multiplayer - High Performance UDP Relay Server
Listens on UDP port 27015 and relays player state packets between Player 1 and Player 2.
"""

import socket
import struct
import time
import sys
from typing import Dict, Optional, Tuple

PROTOCOL_MAGIC = 0x4744414D  # "MADG"
PROTOCOL_VERSION = 1
DEFAULT_PORT = 27015

# Packet Types
PKT_CONNECT_REQ     = 0x01
PKT_CONNECT_ACK     = 0x02
PKT_DISCONNECT      = 0x03
PKT_HEARTBEAT       = 0x04
PKT_POSITION_UPDATE = 0x05

# Header format: uint32 magic, uint16 version, uint8 packetType, uint32 playerId
HEADER_FORMAT = "<IHBI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 11 bytes

# PlayerStatePacket payload: uint32 sequenceNumber, uint8 characterId, float x, float y, float z, float rot, uint32 animState
STATE_FORMAT = "<IBffffI"
STATE_PAYLOAD_SIZE = struct.calcsize(STATE_FORMAT)  # 25 bytes
STATE_PACKET_SIZE = HEADER_SIZE + STATE_PAYLOAD_SIZE  # 36 bytes

CHARACTER_NAMES = {
    0: "Alex",
    1: "Marty",
    2: "Melman",
    3: "Gloria"
}

class ClientSession:
    def __init__(self, player_id: int, slot: int, addr: Tuple[str, int], name: str):
        self.player_id = player_id
        self.slot = slot  # 0 or 1
        self.addr = addr
        self.name = name
        self.last_seen = time.time()
        self.sequence_number = 0

class RelayServer:
    def __init__(self, host: str = "0.0.0.0", port: int = DEFAULT_PORT):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.host, self.port))
        self.sock.setblocking(False)
        self.clients: Dict[int, ClientSession] = {} # player_id -> ClientSession
        self.addr_to_id: Dict[Tuple[str, int], int] = {}
        self.next_player_id = 1001

        print(f"[RELAY] Madagascar (2005) Multiplayer Server running on {self.host}:{self.port}")
        print("[RELAY] Waiting for client connections (Max 2 Players)...")

    def handle_connect_req(self, data: bytes, addr: Tuple[str, int]):
        if len(self.clients) >= 2:
            print(f"[RELAY] Rejecting connect request from {addr}: Server full.")
            ack = struct.pack(
                HEADER_FORMAT + "IB?",
                PROTOCOL_MAGIC, PROTOCOL_VERSION, PKT_CONNECT_ACK, 0,
                0, 0, False
            )
            self.sock.sendto(ack, addr)
            return

        # Player name is 32 bytes after header
        raw_name = data[HEADER_SIZE:HEADER_SIZE + 32]
        player_name = raw_name.split(b'\x00')[0].decode('utf-8', errors='ignore') or f"Player_{self.next_player_id}"
        
        assigned_slot = 0 if not any(c.slot == 0 for c in self.clients.values()) else 1
        assigned_id = self.next_player_id
        self.next_player_id += 1

        session = ClientSession(assigned_id, assigned_slot, addr, player_name)
        self.clients[assigned_id] = session
        self.addr_to_id[addr] = assigned_id

        print(f"[RELAY] Player joined: '{player_name}' (ID: {assigned_id}, Slot: {assigned_slot}) from {addr}")

        # Send ConnectAck
        ack = struct.pack(
            HEADER_FORMAT + "IB?",
            PROTOCOL_MAGIC, PROTOCOL_VERSION, PKT_CONNECT_ACK, assigned_id,
            assigned_id, assigned_slot, True
        )
        self.sock.sendto(ack, addr)

    def handle_disconnect(self, player_id: int, reason: int):
        if player_id in self.clients:
            session = self.clients[player_id]
            print(f"[RELAY] Player disconnected: '{session.name}' (ID: {player_id}, Reason: {reason})")
            if session.addr in self.addr_to_id:
                del self.addr_to_id[session.addr]
            del self.clients[player_id]

    def check_timeouts(self, timeout_sec: float = 5.0):
        now = time.time()
        to_remove = []
        for pid, session in self.clients.items():
            if now - session.last_seen > timeout_sec:
                print(f"[RELAY] Player timed out: '{session.name}' (ID: {pid})")
                to_remove.append(pid)
        for pid in to_remove:
            self.handle_disconnect(pid, reason=1)

    def run(self):
        last_timeout_check = time.time()
        try:
            while True:
                now = time.time()
                if now - last_timeout_check > 1.0:
                    self.check_timeouts()
                    last_timeout_check = now

                try:
                    data, addr = self.sock.recvfrom(2048)
                except BlockingIOError:
                    time.sleep(0.001)  # Sleep 1ms to prevent 100% CPU spinning
                    continue

                if len(data) < HEADER_SIZE:
                    continue

                magic, version, pkt_type, player_id = struct.unpack_from(HEADER_FORMAT, data, 0)
                if magic != PROTOCOL_MAGIC or version != PROTOCOL_VERSION:
                    continue  # Invalid protocol or version

                # Update keepalive for known client
                if addr in self.addr_to_id:
                    client_id = self.addr_to_id[addr]
                    if client_id in self.clients:
                        self.clients[client_id].last_seen = now

                # Dispatch packet types
                if pkt_type == PKT_CONNECT_REQ:
                    self.handle_connect_req(data, addr)
                elif pkt_type == PKT_DISCONNECT:
                    if len(data) >= HEADER_SIZE + 1:
                        reason = data[HEADER_SIZE]
                        self.handle_disconnect(player_id, reason)
                elif pkt_type == PKT_HEARTBEAT:
                    # Echo heartbeat back
                    self.sock.sendto(data, addr)
                elif pkt_type == PKT_POSITION_UPDATE:
                    if len(data) == STATE_PACKET_SIZE and player_id in self.clients:
                        # Relay directly to the other player (Peer-to-Peer Relay)
                        for other_id, other_session in self.clients.items():
                            if other_id != player_id:
                                self.sock.sendto(data, other_session.addr)

        except KeyboardInterrupt:
            print("\n[RELAY] Shutting down server...")
        finally:
            self.sock.close()

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_PORT
    server = RelayServer(port=port)
    server.run()
