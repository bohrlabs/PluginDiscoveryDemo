#pragma once
#include <string>
#include <vector>
#include <cinttypes>

namespace PluginAPI {
    // Enums
    enum struct PortDirection { Input = 0, Output = 1 };
    enum struct PortType { SharedMemory = 0, Socket = 1, Function = 2 };
    enum struct DataAccessPolicy { Direct = 0, Buffered = 1 };

    // Runtime descriptor
    struct PortDescriptor {
        std::string Name;
        PortDirection Direction{};
        PortType Type{};
        DataAccessPolicy AccessPolicy{};
    };

    // Compile-time helpers
    template <size_t N>
    struct ConstexprString {
        char value[N];
        constexpr ConstexprString(const char(&str)[N]) {
            for (size_t i = 0; i < N; ++i) value[i] = str[i];
        }
        constexpr const char* c_str() const { return value; }
    };

    template <
        ConstexprString Name,
        PortDirection Direction,
        PortType Type,
        DataAccessPolicy AccessPolicy
    >
    struct ConstexprPortDescriptor {
        static constexpr auto name = Name;
        static constexpr auto direction = Direction;
        static constexpr auto type = Type;
        static constexpr auto accessPolicy = AccessPolicy;
    };

    template <typename ConstexprDesc>
    PortDescriptor to_runtime_descriptor(ConstexprDesc) {
        PortDescriptor desc;
        desc.Name = ConstexprDesc::name.value;
        desc.Direction = ConstexprDesc::direction;
        desc.Type = ConstexprDesc::type;
        desc.AccessPolicy = ConstexprDesc::accessPolicy;
        return desc;
    }

    // --- New Abstract Plugin Interface ---
    class IPlugin {
    public:
        virtual ~IPlugin() = default;
        virtual std::vector<PortDescriptor> getPortDescriptors() const = 0;
        virtual void initialize() = 0;
        virtual void run() = 0;
        virtual void shutdown() = 0;
    };
}