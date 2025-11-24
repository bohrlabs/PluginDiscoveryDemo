#pragma once
#include "../include/PluginAPI.hpp"

// Concrete plugin implementation for the demo.
class MyAddon: public PluginAPI::IPlugin {
public:
    using InPortT = PluginAPI::AddOnPort<
        "InputSharedMemory",
        PluginAPI::PortDirection::Input,
        PluginAPI::PortType::SharedMemory,
        PluginAPI::DataAccessPolicy::Buffered
    >;

    using OutPortT = PluginAPI::AddOnPort<
        "OutputSharedMemory",
        PluginAPI::PortDirection::Output,
        PluginAPI::PortType::SharedMemory,
        PluginAPI::DataAccessPolicy::Direct
    >;

    MyAddon();

    std::vector<PluginAPI::PortDescriptor> getPortDescriptors() const override;

    void initialize() override;
    void run() override;
    void shutdown() override;

private:
    InPortT  InPort{};
    OutPortT OutPort{};
    PluginAPI::AddOnPort<
        "Input2",
        PluginAPI::PortDirection::Input,
        PluginAPI::PortType::SharedMemory,
        PluginAPI::DataAccessPolicy::Direct>
        InPort2;
};
