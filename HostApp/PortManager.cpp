#include "PortManager.hpp"

using namespace PluginAPI;

void PortManager::BeginAddon(const std::string& addonName) {
    currentAddon_ = addonName;
}

void PortManager::CreatePort(const PortDescriptor& desc) {
    if (currentAddon_.empty()) {
        std::cerr << "[PortManager] CreatePort called without BeginAddon().\n";
        return;
    }

    PortKey key{ currentAddon_, desc.Name };

    if (ports_.contains(key)) {
        std::cerr << "[PortManager] Duplicate port ignored: "
                  << key.addon << "::" << key.port << "\n";
        return;
    }

    PortInfo info;
    info.key  = key;
    info.desc = desc;

    ports_.emplace(key, std::move(info));

    std::cout << "  [PortManager] Registered port " << key.addon << "::" << key.port
              << " | Dir=" << to_string(desc.Direction)
              << " | Type=" << to_string(desc.Type)
              << " | Policy=" << to_string(desc.AccessPolicy)
              << "\n";
}

bool PortManager::Validate(const PortDescriptor& prov,
                           const PortDescriptor& recv,
                           std::string& why) {
    if (prov.Direction != PortDirection::Output) {
        why = "provider is not Output";
        return false;
    }
    if (recv.Direction != PortDirection::Input) {
        why = "receiver is not Input";
        return false;
    }
    if (prov.Type != recv.Type) {
        why = "type mismatch";
        return false;
    }
    // AccessPolicy mismatch may be ok. If you want strict:
    // if (prov.AccessPolicy != recv.AccessPolicy) { why="policy mismatch"; return false; }

    return true;
}

bool PortManager::Connect(const PortKey& provider, const PortKey& receiver) {
    auto pIt = ports_.find(provider);
    auto rIt = ports_.find(receiver);

    if (pIt == ports_.end()) {
        std::cerr << "[PortManager] Provider not found: "
                  << provider.addon << "::" << provider.port << "\n";
        return false;
    }
    if (rIt == ports_.end()) {
        std::cerr << "[PortManager] Receiver not found: "
                  << receiver.addon << "::" << receiver.port << "\n";
        return false;
    }

    std::string why;
    if (!Validate(pIt->second.desc, rIt->second.desc, why)) {
        std::cerr << "[PortManager] Connect failed (" << why << "): "
                  << provider.addon << "::" << provider.port
                  << " -> "
                  << receiver.addon << "::" << receiver.port
                  << "\n";
        return false;
    }

    connections_.push_back({ provider, receiver });

    std::cout << "[PortManager] Connected "
              << provider.addon << "::" << provider.port
              << " -> "
              << receiver.addon << "::" << receiver.port
              << "\n";
    return true;
}

bool PortManager::Connect(const std::string& providerAddon, const std::string& providerPort,
                          const std::string& receiverAddon, const std::string& receiverPort) {
    return Connect(PortKey{providerAddon, providerPort},
                   PortKey{receiverAddon, receiverPort});
}

void PortManager::PrintPorts() const {
    std::cout << "\n[PortManager] Ports:\n";
    for (const auto& [k, info] : ports_) {
        const auto& d = info.desc;
        std::cout << "  " << k.addon << "::" << k.port
                  << " | " << to_string(d.Direction)
                  << " | " << to_string(d.Type)
                  << " | " << to_string(d.AccessPolicy)
                  << "\n";
    }
}

void PortManager::PrintConnections() const {
    std::cout << "\n[PortManager] Connections:\n";
    for (const auto& c : connections_) {
        std::cout << "  " << c.provider.addon << "::" << c.provider.port
                  << " -> "
                  << c.receiver.addon << "::" << c.receiver.port
                  << "\n";
    }
}
