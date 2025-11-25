#pragma once
#include "../include/PluginAPI.hpp"
#include "../include/Packet.hpp"

class MyAddon: public PluginAPI::IPlugin {
    public:
        using OutPortT = PluginAPI::AddOnPort<
            Packet,
            "OutPacket",
            PluginAPI::PortDirection::Output,
            PluginAPI::PortType::SharedMemory,
            PluginAPI::DataAccessPolicy::Direct>;

        std::vector<PluginAPI::PortDescriptor> getPortDescriptors() const override;
        void                                   initialize(PluginAPI::IHostServices *svc) override;
        void                                   run() override;
        void                                   shutdown() override;

    private:
        OutPortT OutPort{};
        Packet   myValue;
        int      tick =11;
};
