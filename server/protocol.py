from enum import IntEnum
import struct

PROTOCOL_MAGIC = 0x4744414D  # "MADG" in ascii
PROTOCOL_VERSION = 1
DEFAULT_PORT = 27015

class DisconnectReason(IntEnum):
    NORMAL = 0
    TIMEOUT = 1
    ERROR = 2

# C = Client
# S = Server
class PacketType(IntEnum):
    # C to S: request to join.
    # Payload:
    #   char[32] - player name
    # Header playerId is 0 since no ID has been assigned yet.
    PKT_CONNECT_REQ     = 0x01

    # S to C: reply to CONNECT_REQ.
    # Payload:
    #   uint32 - assignedPlayerId
    #   uint8 - assignedSlot
    #   bool - success
    PKT_CONNECT_ACK     = 0x02

    # Client <-> Server: player is leaving. Payload: uint8 reason (0 = Normal, 1 = Timeout, 2 = Error).

    # C to S / S to C: PKT_DISCONNECT
    # Payload:
    #   uint8 DisconnectReason - reason
    PKT_DISCONNECT      = 0x03


    # C to S: PKT_HEARTBEAT
    # Payload:
    #   uint32 - timestamp
    # Server echoes it back unchanged
    # Note: Any packet from a known address refreshes its timeout.
    PKT_HEARTBEAT       = 0x04

    # TODO
    # Client -> Server -> other client: player state at ~60 Hz (see STATE_FORMAT).
    # The server doesn't parse the payload, it only checks the size and forwards it.

    # C to S to Other Clients: PKT_POSITION_UPDATE
    # Payload:
    #   uint32 sequenceNumber
    #   uint8 characterId
    #   float x
    #   float y
    #   float z
    #   float rot
    #   uint32 animState
    PKT_POSITION_UPDATE = 0x05


# Header format: uint32 magic, uint16 version, uint8 packetType, uint32 playerId
HEADER_FORMAT = "<IHBI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 11 bytes

# PlayerStatePacket payload: uint32 sequenceNumber, uint8 characterId, float x, float y, float z, float rot, uint32 animState
STATE_FORMAT = "<IBffffI"
STATE_PAYLOAD_SIZE = struct.calcsize(STATE_FORMAT)  # 25 bytes
STATE_PACKET_SIZE = HEADER_SIZE + STATE_PAYLOAD_SIZE  # 36 bytes
