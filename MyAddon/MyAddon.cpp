#include "PluginAPI.hpp"

// Define ports at compile time
using InPort = PluginAPI::ConstexprPortDescriptor<
    "InputSharedMemory",
    PluginAPI::PortDirection::Input,
    PluginAPI::PortType::SharedMemory,
    PluginAPI::DataAccessPolicy::Buffered
>;

using OutPort = PluginAPI::ConstexprPortDescriptor<
    "OutputSharedMemory",
    PluginAPI::PortDirection::Output,
    PluginAPI::PortType::SharedMemory,
    PluginAPI::DataAccessPolicy::Direct
>;



PLUGIN_EXPORT
std::vector<PluginAPI::PortDescriptor> GetPortDescriptors() {
    return {
        PluginAPI::to_runtime_descriptor(InPort{}),
        PluginAPI::to_runtime_descriptor(OutPort{})
    };
}