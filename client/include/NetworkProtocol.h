#pragma once
#include <cstdint>

namespace MadMultiplayer::Network {

    // Magic identifier to reject invalid/garbage UDP datagrams: "MADG" (0x4744414D)
    inline constexpr uint32_t PROTOCOL_MAGIC = 0x4744414D;
    inline constexpr uint16_t PROTOCOL_VERSION = 1;
    inline constexpr uint16_t DEFAULT_PORT = 27015;

    enum class PacketType : uint8_t {
        ConnectReq     = 0x01, // Client -> Server
        ConnectAck     = 0x02, // Server -> Client
        Disconnect     = 0x03, // Client <-> Server
        Heartbeat      = 0x04, // Keepalive
        PositionUpdate = 0x05  // High frequency state sync (60 Hz)
    };

    enum class CharacterType : uint8_t {
        Alex    = 0,
        Marty   = 1,
        Melman  = 2,
        Gloria  = 3,
        Unknown = 255
    };

#pragma pack(push, 1)

    // Standard Header for every packet
    struct PacketHeader {
        uint32_t magic;         // PROTOCOL_MAGIC ("MADG")
        uint16_t version;       // PROTOCOL_VERSION
        uint8_t  packetType;    // PacketType
        uint32_t playerId;      // Assigned player ID (or 0 during ConnectReq)
    };

    // Client connection request
    struct ConnectReqPacket {
        PacketHeader header;
        char         playerName[32];
    };

    // Server acknowledgement with assigned player slot/ID
    struct ConnectAckPacket {
        PacketHeader header;
        uint32_t     assignedPlayerId;
        uint8_t      assignedSlot; // 0 = Player 1, 1 = Player 2
        bool         success;
    };

    // Keepalive ping / pong
    struct HeartbeatPacket {
        PacketHeader header;
        uint32_t     timestamp;
    };

    // High frequency position & state synchronisation (60 Hz)
    struct PlayerStatePacket {
        PacketHeader header;
        uint32_t     sequenceNumber; // Monotonically increasing sequence number
        uint8_t      characterId;    // 0 = Alex, 1 = Marty, 2 = Melman, 3 = Gloria
        float        position[3];    // X, Y (RW Z-Height), Z
        float        rotation;       // Yaw in radians or degrees
        uint32_t     animationState; // Reserved for animation ID / locomotion index
    };

    // Disconnect packet
    struct DisconnectPacket {
        PacketHeader header;
        uint8_t      reason; // 0 = Normal, 1 = Timeout, 2 = Error
    };

#pragma pack(pop)

    // Compile-time checks for struct alignment and size consistency
    static_assert(sizeof(PacketHeader) == 11, "PacketHeader size mismatch");
    static_assert(sizeof(PlayerStatePacket) == 11 + 4 + 1 + 12 + 4 + 4, "PlayerStatePacket size mismatch (expected 36 bytes)");

} // namespace MadMultiplayer::Network
