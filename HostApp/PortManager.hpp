#pragma once
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include "../include/PluginAPI.hpp"
#include "AddOnManager.hpp" // for IHostPortServices

class PortManager: public IHostPortServices {
public:
    struct PortKey {
        std::string addon;  // e.g. "MyAddon2"
        std::string port;   // e.g. "OutputSharedMemory"

        bool operator<(const PortKey& o) const {
            if (addon!=o.addon) return addon<o.addon;
            return port<o.port;
        }
    };

    struct PortInfo {
        PortKey                 key;
        PluginAPI::PortDescriptor desc;
        // later: transport handle / shared memory ptr / queue id, etc.
        void* transport = nullptr;
    };

    struct Connection {
        PortKey provider; // Output
        PortKey receiver; // Input
    };

    // Called by AddOnManager before pushing ports of one addon
    void BeginAddon(const std::string& addonName);

    // IHostPortServices
    void CreatePort(const PluginAPI::PortDescriptor& desc) override;

    // Connect by keys
    bool Connect(const PortKey& provider, const PortKey& receiver);

    // Convenience connect by names
    bool Connect(const std::string& providerAddon, const std::string& providerPort,
        const std::string& receiverAddon, const std::string& receiverPort);

    const std::map<PortKey, PortInfo>& ports() const { return ports_; }
    const std::vector<Connection>& connections() const { return connections_; }

    void PrintPorts() const;
    void PrintConnections() const;

private:
    static bool Validate(const PluginAPI::PortDescriptor& prov,
        const PluginAPI::PortDescriptor& recv,
        std::string& why);

    std::string currentAddon_;
    std::map<PortKey, PortInfo> ports_;
    std::vector<Connection> connections_;
};
