#pragma once
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include "../include/PluginAPI.hpp"
#include "AddOnManager.hpp" // for IHostPortServices

class PortManager: public IHostPortServices, public PluginAPI::IHostServices {
    public:
        struct PortKey {
                std::string addon;
                std::string port;
                bool        operator<(const PortKey &o) const {
                    if (addon != o.addon)
                        return addon < o.addon;
                    return port < o.port;
                }
        };

        struct PortInfo {
                PortKey                   key;
                PluginAPI::PortDescriptor desc;
                void                     *transport = nullptr; // used for Direct; null for Buffered
        };

        struct Connection {
                PortKey provider;
                PortKey receiver;

                // For buffered ports:
                std::vector<std::uint8_t> buffer;
                bool                      hasData = false;
        };

        // Called by AddOnManager before pushing ports of one addon
        void BeginAddon(const std::string &addonName);

        // IHostPortServices
        void CreatePort(const PluginAPI::PortDescriptor &desc) override;

        // Connect by keys
        bool Connect(const PortKey &provider, const PortKey &receiver);

        // Convenience connect by names
        bool Connect(const std::string &providerAddon, const std::string &providerPort,
            const std::string &receiverAddon, const std::string &receiverPort);

        const std::map<PortKey, PortInfo> &ports() const {
            return ports_;
        }
        const std::vector<Connection> &connections() const {
            return connections_;
        }

        void PrintPorts() const;
        void PrintConnections() const;

        PluginAPI::PortHandle OpenPort(const char *name) override;
        bool                  Read(PluginAPI::PortHandle h, void *dst, size_t bytes, size_t &outBytes) override;
        bool                  Write(PluginAPI::PortHandle h, const void *src, size_t bytes, size_t &outBytes) override;

        // Project functionalities
        bool SaveToFile(const std::string &filename) const;
        bool LoadFromFile(const std::string &filename);

    private:
        static bool Validate(const PluginAPI::PortDescriptor &prov,
            const PluginAPI::PortDescriptor                  &recv,
            std::string                                      &why);

        std::string                 currentAddon_;
        std::map<PortKey, PortInfo> ports_;
        std::vector<Connection>     connections_;
};
