"""
Madagascar (2005) Multiplayer - High Performance UDP Relay Server
Listens on UDP port 27015 and relays player state packets between Player 1 and Player 2.
"""

import socket
import time
from protocol import DEFAULT_PORT, DisconnectReason
from packets import (
    ConnectAckPacket,
    ConnectReqPacket,
    DisconnectPacket,
    HeartbeatPacket,
    PacketError,
    PlayerStatePacket,
    decode,
    encode,
)
import argparse


class ClientSession:
    def __init__(self, player_id: int, slot: int, addr: tuple[str, int], name: str):
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
        self.clients: dict[int, ClientSession] = {}  # player_id -> ClientSession
        self.addr_to_id: dict[tuple[str, int], int] = {}
        self.next_player_id = 1001

        print(
            f"[RELAY] Madagascar (2005) Multiplayer Server running on {self.host}:{self.port}"
        )
        print("[RELAY] Waiting for client connections (Max 2 Players)...")

    def handle_connect_req(self, packet: ConnectReqPacket, addr: tuple[str, int]):
        if len(self.clients) >= 2:
            print(f"[RELAY] Rejecting connect request from {addr}: Server full.")
            self.sock.sendto(encode(0, ConnectAckPacket(0, 0, False)), addr)
            return

        player_name = packet.player_name or f"Player_{self.next_player_id}"

        assigned_slot = 0 if not any(c.slot == 0 for c in self.clients.values()) else 1
        assigned_id = self.next_player_id
        self.next_player_id += 1

        session = ClientSession(assigned_id, assigned_slot, addr, player_name)
        self.clients[assigned_id] = session
        self.addr_to_id[addr] = assigned_id

        print(
            f"[RELAY] Player joined: '{player_name}' (ID: {assigned_id}, Slot: {assigned_slot}) from {addr}"
        )

        ack = ConnectAckPacket(assigned_id, assigned_slot, True)
        self.sock.sendto(encode(assigned_id, ack), addr)

    def handle_disconnect(self, player_id: int, reason: DisconnectReason):
        if player_id in self.clients:
            session = self.clients[player_id]
            print(
                f"[RELAY] Player disconnected: '{session.name}' (ID: {player_id}, Reason: {reason.name})"
            )
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
            self.handle_disconnect(pid, DisconnectReason.TIMEOUT)

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

                try:
                    header, packet = decode(data)
                except PacketError:
                    continue  # Invalid protocol, version, type or size

                # Update keepalive for known client
                if addr in self.addr_to_id:
                    client_id = self.addr_to_id[addr]
                    if client_id in self.clients:
                        self.clients[client_id].last_seen = now

                # Dispatch packet types
                match packet:
                    case ConnectReqPacket():
                        self.handle_connect_req(packet, addr)
                    case DisconnectPacket():
                        self.handle_disconnect(header.player_id, packet.reason)
                    case HeartbeatPacket():
                        # Echo heartbeat back unchanged
                        self.sock.sendto(data, addr)
                    case PlayerStatePacket() if header.player_id in self.clients:
                        # Relay the raw datagram to the other player (Peer-to-Peer Relay)
                        for other_id, other_session in self.clients.items():
                            if other_id != header.player_id:
                                self.sock.sendto(data, other_session.addr)

        except KeyboardInterrupt:
            print("\n[RELAY] Shutting down server...")
        finally:
            self.sock.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--port",
        help="The Port the server will listen on",
        type=int,
        default=DEFAULT_PORT,
    )
    args = parser.parse_args()

    server = RelayServer(port=args.port)
    server.run()
