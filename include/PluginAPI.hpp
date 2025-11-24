#pragma once
#include <string>
#include <vector>
#include <cinttypes>

#if defined(_WIN32) || defined(_WIN64)
    #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define PLUGIN_EXPORT extern "C"
#endif
// Namespace for plugin framework
namespace PluginAPI {

    // Enums for port configuration
    enum struct PortDirection {
        Input  = 0,
        Output = 1
    };
    enum struct PortType {
        SharedMemory = 0,
        Socket       = 1,
        Function     = 2
    };
    enum struct DataAccessPolicy {
        Direct   = 0,
        Buffered = 1,
    };

    // Runtime port descriptor
    struct PortDescriptor {
        std::string      Name;
        PortDirection    Direction{};
        PortType         Type{};
        DataAccessPolicy AccessPolicy{};
    };

    // Compile-time string wrapper
    template <size_t N>
    struct ConstexprString {
        char value[N];
        constexpr ConstexprString(const char(&str)[N]) {
            for (size_t i = 0; i < N; ++i) value[i] = str[i];
        }
        constexpr const char* c_str() const { return value; }
    };

    // Compile-time port descriptor type
    template <
        ConstexprString Name,
        PortDirection Direction,
        PortType Type,
        DataAccessPolicy AccessPolicy
    >
    struct ConstexprPortDescriptor {
        static constexpr auto name         = Name;
        static constexpr auto direction    = Direction;
        static constexpr auto type         = Type;
        static constexpr auto accessPolicy = AccessPolicy;
    };

    // Convert compile-time descriptor to runtime descriptor
    template <typename ConstexprDesc>
    PortDescriptor to_runtime_descriptor(ConstexprDesc) {
        PortDescriptor desc;
        desc.Name         = ConstexprDesc::name.value;
        desc.Direction    = ConstexprDesc::direction;
        desc.Type         = ConstexprDesc::type;
        desc.AccessPolicy = ConstexprDesc::accessPolicy;
        return desc;
    }
}