#pragma once
#include "../include/PluginAPI.hpp"
#include "../include/Packet.hpp"

class MyAddon2: public PluginAPI::IPlugin {
    public:
        using InPortT = PluginAPI::AddOnPort<
            Packet,
            "InPacket",
            PluginAPI::PortDirection::Input,
            PluginAPI::PortType::SharedMemory,
            PluginAPI::DataAccessPolicy::Direct>;

        using OutPortT = PluginAPI::AddOnPort<
            Packet,
            "ProcessedPacket",
            PluginAPI::PortDirection::Output,
            PluginAPI::PortType::SharedMemory,
            PluginAPI::DataAccessPolicy::Direct>;

        std::vector<PluginAPI::PortDescriptor> getPortDescriptors() const override;
        void                                   initialize(PluginAPI::IHostServices *svc) override;
        void                                   run() override;
        void                                   shutdown() override;

    private:
        InPortT  InPort{};
        OutPortT OutPort{};
        Packet   myStuff;
};
