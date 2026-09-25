import struct
from dataclasses import dataclass
from typing import Self

from protocol import (
    PROTOCOL_MAGIC,
    PROTOCOL_VERSION,
    HEADER_FORMAT,
    STATE_FORMAT,
    DisconnectReason,
    PacketType,
)

PLAYER_NAME_SIZE = 32


class PacketError(ValueError):
    """Raised when a datagram can't be decoded into a packet."""


@dataclass(frozen=True)
class Header:
    FORMAT = struct.Struct(HEADER_FORMAT)

    packet_type: PacketType
    player_id: int = 0  # 0 until the server has assigned one

    def to_bytes(self) -> bytes:
        return self.FORMAT.pack(
            PROTOCOL_MAGIC, PROTOCOL_VERSION, self.packet_type, self.player_id
        )

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        if len(data) < cls.FORMAT.size:
            raise PacketError(f"Datagram too short for header: {len(data)} bytes")
        magic, version, packet_type, player_id = cls.FORMAT.unpack_from(data)
        if magic != PROTOCOL_MAGIC or version != PROTOCOL_VERSION:
            raise PacketError(f"Invalid magic/version: {magic:#x}/{version}")
        try:
            return cls(PacketType(packet_type), player_id)
        except ValueError:
            raise PacketError(f"Unknown packet type: {packet_type:#x}") from None


@dataclass
class ConnectReqPacket:
    TYPE = PacketType.PKT_CONNECT_REQ
    FORMAT = struct.Struct(f"<{PLAYER_NAME_SIZE}s")

    player_name: str

    def to_bytes(self) -> bytes:
        # Leave room for the null terminator the client expects in char[32].
        return self.FORMAT.pack(
            self.player_name.encode("utf-8")[: PLAYER_NAME_SIZE - 1]
        )

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        (raw_name,) = cls.FORMAT.unpack_from(data)
        return cls(raw_name.split(b"\x00")[0].decode("utf-8", errors="ignore"))


@dataclass
class ConnectAckPacket:
    TYPE = PacketType.PKT_CONNECT_ACK
    FORMAT = struct.Struct("<IB?")

    assigned_player_id: int
    assigned_slot: int
    success: bool

    def to_bytes(self) -> bytes:
        return self.FORMAT.pack(
            self.assigned_player_id, self.assigned_slot, self.success
        )

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        return cls(*cls.FORMAT.unpack_from(data))


@dataclass
class DisconnectPacket:
    TYPE = PacketType.PKT_DISCONNECT
    FORMAT = struct.Struct("<B")

    reason: DisconnectReason

    def to_bytes(self) -> bytes:
        return self.FORMAT.pack(self.reason)

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        (reason,) = cls.FORMAT.unpack_from(data)
        return cls(DisconnectReason(reason))


@dataclass
class HeartbeatPacket:
    TYPE = PacketType.PKT_HEARTBEAT
    FORMAT = struct.Struct("<I")

    timestamp: int

    def to_bytes(self) -> bytes:
        return self.FORMAT.pack(self.timestamp)

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        return cls(*cls.FORMAT.unpack_from(data))


@dataclass
class PlayerStatePacket:
    TYPE = PacketType.PKT_POSITION_UPDATE
    FORMAT = struct.Struct(STATE_FORMAT)

    sequence_number: int
    character_id: int
    x: float
    y: float
    z: float
    rotation: float
    anim_state: int

    def to_bytes(self) -> bytes:
        return self.FORMAT.pack(
            self.sequence_number,
            self.character_id,
            self.x,
            self.y,
            self.z,
            self.rotation,
            self.anim_state,
        )

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        return cls(*cls.FORMAT.unpack_from(data))


type Packet = (
    ConnectReqPacket
    | ConnectAckPacket
    | DisconnectPacket
    | HeartbeatPacket
    | PlayerStatePacket
)

PACKET_CLASSES: dict[PacketType, type[Packet]] = {
    PacketType.PKT_CONNECT_REQ: ConnectReqPacket,
    PacketType.PKT_CONNECT_ACK: ConnectAckPacket,
    PacketType.PKT_DISCONNECT: DisconnectPacket,
    PacketType.PKT_HEARTBEAT: HeartbeatPacket,
    PacketType.PKT_POSITION_UPDATE: PlayerStatePacket,
}


def encode(player_id: int, packet: Packet) -> bytes:
    """Serialize a packet with its header, ready to send."""
    return Header(packet.TYPE, player_id).to_bytes() + packet.to_bytes()

def decode(data: bytes) -> tuple[Header, Packet]:
    """Split a raw datagram into its header and decoded payload."""
    header = Header.from_bytes(data)
    packet_cls = PACKET_CLASSES[header.packet_type]
    try:
        packet = packet_cls.from_bytes(data[Header.FORMAT.size :])
    except (struct.error, ValueError) as e:
        raise PacketError(f"Malformed {packet_cls.__name__}: {e}") from None
    return header, packet
