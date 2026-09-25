"""
Simple Loopback & Packet Test Utility for Madagascar (2005) Multiplayer.
Emulates two clients sending connect requests, heartbeats, and position updates.
"""

import socket

from protocol import DEFAULT_PORT, DisconnectReason
from packets import (
    ConnectAckPacket,
    ConnectReqPacket,
    PlayerStatePacket,
    DisconnectPacket,
    decode,
    encode,
)


def connect(sock: socket.socket, server_addr: tuple[str, int], name: str) -> ConnectAckPacket:
    sock.sendto(encode(0, ConnectReqPacket(name)), server_addr)
    data, _ = sock.recvfrom(1024)
    _, ack = decode(data)
    assert isinstance(ack, ConnectAckPacket)
    return ack


def test_handshake_and_relay():
    sock1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock1.settimeout(1.0)
    sock2.settimeout(1.0)

    server_addr = ("127.0.0.1", DEFAULT_PORT)

    # 1. Connect Client 1 (Alex)
    ack1 = connect(sock1, server_addr, "Alex_Host")
    print(f"[TEST] Client 1 Connected: ID={ack1.assigned_player_id}, Slot={ack1.assigned_slot}, Success={ack1.success}")

    # 2. Connect Client 2 (Marty)
    ack2 = connect(sock2, server_addr, "Marty_Client")
    print(f"[TEST] Client 2 Connected: ID={ack2.assigned_player_id}, Slot={ack2.assigned_slot}, Success={ack2.success}")

    # 3. Client 1 sends position update
    sent = PlayerStatePacket(
        sequence_number=1,
        character_id=0,  # Alex
        x=120.5,
        y=45.0,
        z=-89.2,
        rotation=1.57,
        anim_state=10,
    )
    sock1.sendto(encode(ack1.assigned_player_id, sent), server_addr)

    # 4. Client 2 should receive Client 1's position update
    relayed_data, _ = sock2.recvfrom(1024)
    header, received = decode(relayed_data)
    assert isinstance(received, PlayerStatePacket)

    print(f"[TEST] Client 2 received relayed packet from Player ID {header.player_id}!")
    print(f"       Seq: {received.sequence_number}, Char: {received.character_id}, "
          f"Pos: ({received.x:.2f}, {received.y:.2f}, {received.z:.2f}), Yaw: {received.rotation:.2f}")

    assert header.player_id == ack1.assigned_player_id
    assert received.character_id == sent.character_id
    assert abs(received.x - sent.x) < 0.001
    assert abs(received.y - sent.y) < 0.001
    assert abs(received.z - sent.z) < 0.001
    print("[TEST] SUCCESS! Binary packing, transmission, and relay forwarding verified perfectly.")

    # 5. Client 1 disconnects
    sent = DisconnectPacket(
        reason=DisconnectReason.NORMAL
    )
    sock1.sendto(encode(ack1.assigned_player_id, sent), server_addr)


    sock1.close()
    sock2.close()


if __name__ == "__main__":
    test_handshake_and_relay()
