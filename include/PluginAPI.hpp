#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace PluginAPI {

    // Enums
    enum struct PortDirection: std::uint8_t { Input = 0, Output = 1 };
    enum struct PortType: std::uint8_t { SharedMemory = 0, Socket = 1, Function = 2 };
    enum struct DataAccessPolicy: std::uint8_t { Direct = 0, Buffered = 1 };

    // Runtime descriptor
    struct PortDescriptor {
        std::string     Name;
        PortDirection   Direction{};
        PortType        Type{};
        DataAccessPolicy AccessPolicy{};
    };

    // Helper to stringify enums for printing (host-side convenience)
    inline const char* to_string(PortDirection dir) {
        switch (dir) {
        case PortDirection::Input:  return "Input";
        case PortDirection::Output: return "Output";
        default: return "Unknown";
        }
    }
    inline const char* to_string(PortType type) {
        switch (type) {
        case PortType::SharedMemory: return "SharedMemory";
        case PortType::Socket:       return "Socket";
        case PortType::Function:     return "Function";
        default: return "Unknown";
        }
    }
    inline const char* to_string(DataAccessPolicy pol) {
        switch (pol) {
        case DataAccessPolicy::Direct:   return "Direct";
        case DataAccessPolicy::Buffered: return "Buffered";
        default: return "Unknown";
        }
    }

    // Compile-time fixed string for NTTP
    template <std::size_t N>
    struct fixed_string {
        char v[N];
        constexpr fixed_string(const char(&s)[N]) {
            for (std::size_t i = 0; i<N; ++i) v[i] = s[i];
        }
        constexpr const char* c_str() const { return v; }
        constexpr std::size_t size() const { return N; }
    };

    // Runtime port object with compile-time identity.
    // Identity fields are constexpr; Data is runtime-owned by the addon.
    template <
        fixed_string Name,
        PortDirection Direction,
        PortType Type,
        DataAccessPolicy AccessPolicy
    >
    class AddOnPort {
    public:
        static constexpr auto name = Name;
        static constexpr auto direction = Direction;
        static constexpr auto type = Type;
        static constexpr auto accessPolicy = AccessPolicy;

        void* Data = nullptr; // runtime attachment/handle owned by plugin

        AddOnPort() = default;

        // Implicit conversion to runtime descriptor so callers don't need
        // to call a helper like to_runtime_descriptor().
        operator PortDescriptor() const {
            return PortDescriptor{
                name.c_str(),
                direction,
                type,
                accessPolicy
            };
        }
    };

    // --- Abstract Plugin Interface ---
    class IPlugin {
    public:
        virtual ~IPlugin() = default;

        [[nodiscard]] virtual std::vector<PortDescriptor> getPortDescriptors() const = 0;
        virtual void initialize() = 0;
        virtual void run() = 0;
        virtual void shutdown() = 0;
    };

} // namespace PluginAPI
