#!/usr/bin/env python3
"""
Simple Loopback & Packet Test Utility for Madagascar (2005) Multiplayer.
Emulates two clients sending connect requests, heartbeats, and position updates.
"""

import socket
import struct
import time

PROTOCOL_MAGIC = 0x4744414D  # "MADG"
PROTOCOL_VERSION = 1
DEFAULT_PORT = 27015

HEADER_FORMAT = "<IHBI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
STATE_FORMAT = "<IBffffI"

def test_handshake_and_relay():
    sock1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock1.settimeout(1.0)
    sock2.settimeout(1.0)

    server_addr = ("127.0.0.1", DEFAULT_PORT)

    # 1. Connect Client 1 (Alex)
    p1_name = b"Alex_Host\x00" + b"\x00" * (32 - len("Alex_Host\x00"))
    pkt1 = struct.pack(HEADER_FORMAT, PROTOCOL_MAGIC, PROTOCOL_VERSION, 0x01, 0) + p1_name
    sock1.sendto(pkt1, server_addr)
    ack1_data, _ = sock1.recvfrom(1024)
    _, _, _, _, p1_id, p1_slot, ok1 = struct.unpack(HEADER_FORMAT + "IB?", ack1_data)
    print(f"[TEST] Client 1 Connected: ID={p1_id}, Slot={p1_slot}, Success={ok1}")

    # 2. Connect Client 2 (Marty)
    p2_name = b"Marty_Client\x00" + b"\x00" * (32 - len("Marty_Client\x00"))
    pkt2 = struct.pack(HEADER_FORMAT, PROTOCOL_MAGIC, PROTOCOL_VERSION, 0x01, 0) + p2_name
    sock2.sendto(pkt2, server_addr)
    ack2_data, _ = sock2.recvfrom(1024)
    _, _, _, _, p2_id, p2_slot, ok2 = struct.unpack(HEADER_FORMAT + "IB?", ack2_data)
    print(f"[TEST] Client 2 Connected: ID={p2_id}, Slot={p2_slot}, Success={ok2}")

    # 3. Client 1 sends position update
    seq = 1
    char_id = 0  # Alex
    x, y, z = 120.5, 45.0, -89.2
    yaw = 1.57
    anim = 10
    state_payload = struct.pack(STATE_FORMAT, seq, char_id, x, y, z, yaw, anim)
    state_header = struct.pack(HEADER_FORMAT, PROTOCOL_MAGIC, PROTOCOL_VERSION, 0x05, p1_id)
    sock1.sendto(state_header + state_payload, server_addr)

    # 4. Client 2 should receive Client 1's position update
    relayed_data, _ = sock2.recvfrom(1024)
    _, _, _, rel_pid = struct.unpack_from(HEADER_FORMAT, relayed_data, 0)
    r_seq, r_char, r_x, r_y, r_z, r_yaw, r_anim = struct.unpack_from(STATE_FORMAT, relayed_data, HEADER_SIZE)

    print(f"[TEST] Client 2 received relayed packet from Player ID {rel_pid}!")
    print(f"       Seq: {r_seq}, Char: {r_char}, Pos: ({r_x:.2f}, {r_y:.2f}, {r_z:.2f}), Yaw: {r_yaw:.2f}")

    assert rel_pid == p1_id
    assert r_char == 0
    assert abs(r_x - x) < 0.001
    assert abs(r_y - y) < 0.001
    assert abs(r_z - z) < 0.001
    print("[TEST] SUCCESS! Binary packing, transmission, and relay forwarding verified perfectly.")

    sock1.close()
    sock2.close()

if __name__ == "__main__":
    test_handshake_and_relay()
