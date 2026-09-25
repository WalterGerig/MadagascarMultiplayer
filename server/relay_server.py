"""
Madagascar (2005) Multiplayer - UDP Relay Server
Listens on UDP port 27015 and relays player state packets between Player 1 and Player 2.
"""

import argparse
import socket
import time
from dataclasses import dataclass, field

from packets import (
    ConnectAckPacket,
    ConnectReqPacket,
    DisconnectPacket,
    HeartbeatPacket,
    Packet,
    PacketError,
    PlayerStatePacket,
    decode,
    encode,
)
from protocol import DEFAULT_PORT, DisconnectReason

type Address = tuple[str, int]

MAX_PLAYERS = 2
FIRST_PLAYER_ID = 1001
CLIENT_TIMEOUT_SEC = 5.0
TIMEOUT_CHECK_INTERVAL_SEC = 1.0
RECV_BUFFER_SIZE = 2048


@dataclass
class ClientSession:
    player_id: int
    slot: int  # 0 .. MAX_PLAYERS - 1
    addr: Address
    name: str
    last_seen: float = field(default_factory=time.monotonic)


class RelayServer:
    def __init__(self, host: str = "0.0.0.0", port: int = DEFAULT_PORT):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((host, port))
        # Block on recv, but wake up regularly to check for timed out clients.
        self.sock.settimeout(TIMEOUT_CHECK_INTERVAL_SEC)
        self.sessions: dict[Address, ClientSession] = {}
        self.next_player_id = FIRST_PLAYER_ID

        print(f"[RELAY] Madagascar (2005) Multiplayer Server running on {host}:{port}")
        print(f"[RELAY] Waiting for client connections (Max {MAX_PLAYERS} Players)...")

    def send(self, player_id: int, packet: Packet, addr: Address):
        self.sock.sendto(encode(player_id, packet), addr)

    def handle_connect_req(self, packet: ConnectReqPacket, addr: Address):
        # Client retried because our ACK got lost: answer with the existing session.
        if session := self.sessions.get(addr):
            self.send(session.player_id, ConnectAckPacket(session.player_id, session.slot, True), addr)
            return

        if len(self.sessions) >= MAX_PLAYERS:
            print(f"[RELAY] Rejecting connect request from {addr}: Server full.")
            self.send(0, ConnectAckPacket(0, 0, False), addr)
            return

        used_slots = {s.slot for s in self.sessions.values()}
        slot = next(i for i in range(MAX_PLAYERS) if i not in used_slots)
        player_id = self.next_player_id
        self.next_player_id += 1
        name = packet.player_name or f"Player_{player_id}"

        self.sessions[addr] = ClientSession(player_id, slot, addr, name)
        print(f"[RELAY] Player joined: '{name}' (ID: {player_id}, Slot: {slot}) from {addr}")
        self.send(player_id, ConnectAckPacket(player_id, slot, True), addr)

    def disconnect(self, session: ClientSession, reason: DisconnectReason):
        del self.sessions[session.addr]
        print(
            f"[RELAY] Player disconnected: '{session.name}' "
            f"(ID: {session.player_id}, Reason: {reason.name})"
        )

    def relay_to_others(self, data: bytes, sender: ClientSession):
        for session in self.sessions.values():
            if session is not sender:
                self.sock.sendto(data, session.addr)

    def check_timeouts(self):
        now = time.monotonic()
        for session in list(self.sessions.values()):
            if now - session.last_seen > CLIENT_TIMEOUT_SEC:
                self.disconnect(session, DisconnectReason.TIMEOUT)

    def handle_datagram(self, data: bytes, addr: Address):
        try:
            header, packet = decode(data)
        except PacketError:
            return  # Invalid protocol, version, type or size

        if isinstance(packet, ConnectReqPacket):
            self.handle_connect_req(packet, addr)
            return

        # Everything else is only accepted from a connected client using its own ID.
        session = self.sessions.get(addr)
        if session is None or header.player_id != session.player_id:
            return
        session.last_seen = time.monotonic()

        match packet:
            case DisconnectPacket():
                self.disconnect(session, packet.reason)
            case HeartbeatPacket():
                self.sock.sendto(data, addr)  # Echo back unchanged
            case PlayerStatePacket():
                self.relay_to_others(data, session)

    def run(self):
        last_timeout_check = time.monotonic()
        try:
            while True:
                try:
                    data, addr = self.sock.recvfrom(RECV_BUFFER_SIZE)
                    self.handle_datagram(data, addr)
                except TimeoutError:
                    pass
                except ConnectionResetError:
                    # Windows reports ICMP "port unreachable" from an earlier sendto here.
                    pass

                now = time.monotonic()
                if now - last_timeout_check >= TIMEOUT_CHECK_INTERVAL_SEC:
                    self.check_timeouts()
                    last_timeout_check = now
        except KeyboardInterrupt:
            print("\n[RELAY] Shutting down server...")
        finally:
            self.sock.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Madagascar (2005) Multiplayer relay server")
    parser.add_argument("--host", default="0.0.0.0", help="The address the server will bind to")
    parser.add_argument(
        "--port", type=int, default=DEFAULT_PORT, help="The port the server will listen on"
    )
    args = parser.parse_args()

    RelayServer(args.host, args.port).run()
