#include "PortManager.hpp"

using namespace PluginAPI;

void PortManager::BeginAddon(const std::string &addonName) {
    currentAddon_ = addonName;
}

void PortManager::CreatePort(const PortDescriptor &desc) {
    if (currentAddon_.empty()) {
        std::cerr << "[PortManager] CreatePort called without BeginAddon().\n";
        return;
    }

    PortKey key{currentAddon_, desc.Name};

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

bool PortManager::Validate(const PortDescriptor &prov,
    const PortDescriptor                        &recv,
    std::string                                 &why) {
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

bool PortManager::Connect(const PortKey &provider, const PortKey &receiver) {
    auto pIt = ports_.find(provider);
    auto rIt = ports_.find(receiver);
    if (pIt == ports_.end() || rIt == ports_.end())
        return false;

    auto &prov = pIt->second;
    auto &recv = rIt->second;

    // Validate basic properties
    std::string why;
    if (!Validate(prov.desc, recv.desc, why)) {
        std::cerr << "[PortManager] Connect failed: " << why << "\n";
        return false;
    }

    if (prov.desc.PayloadSize != recv.desc.PayloadSize ||
        prov.desc.TypeHash != recv.desc.TypeHash) {
        std::cerr << "[PortManager] Connect failed: payload mismatch\n";
        return false;
    }

    // For now: require same access policy for buffered connections
    if (prov.desc.AccessPolicy != recv.desc.AccessPolicy) {
        std::cerr << "[PortManager] Connect failed: access policy mismatch "
                  << "(Buffered<->Direct not yet supported)\n";
        return false;
    }

    Connection conn;
    conn.provider = provider;
    conn.receiver = receiver;

    if (prov.desc.AccessPolicy == PluginAPI::DataAccessPolicy::Direct) {
        // DIRECT: shared memory
        if (!prov.transport) {
            prov.transport = ::operator new(prov.desc.PayloadSize);
            std::memset(prov.transport, 0, prov.desc.PayloadSize);
        }
        recv.transport = prov.transport;
        // buffer unused for direct
    } else {
        // BUFFERED: allocate per-connection buffer
        conn.buffer.resize(prov.desc.PayloadSize);
        conn.hasData = false;
    }

    connections_.push_back(std::move(conn));
    return true;
}

bool PortManager::Connect(const std::string &providerAddon, const std::string &providerPort,
    const std::string &receiverAddon, const std::string &receiverPort) {
    return Connect(PortKey{providerAddon, providerPort},
        PortKey{receiverAddon, receiverPort});
}

void PortManager::PrintPorts() const {
    std::cout << "\n[PortManager] Ports:\n";
    for (const auto &[k, info] : ports_) {
        const auto &d = info.desc;
        std::cout << "  " << k.addon << "::" << k.port
                  << " | " << to_string(d.Direction)
                  << " | " << to_string(d.Type)
                  << " | " << to_string(d.AccessPolicy)
                  << "\n";
    }
}

void PortManager::PrintConnections() const {
    std::cout << "\n[PortManager] Connections:\n";
    for (const auto &c : connections_) {
        std::cout << "  " << c.provider.addon << "::" << c.provider.port
                  << " -> "
                  << c.receiver.addon << "::" << c.receiver.port
                  << "\n";
    }
}
PluginAPI::PortHandle PortManager::OpenPort(const char *name) {
    PortKey key{currentAddon_, name};
    auto    it = ports_.find(key);
    if (it == ports_.end())
        return {};

    auto &pi = it->second;

    if (pi.desc.AccessPolicy == PluginAPI::DataAccessPolicy::Direct) {
        // Direct – transport set in Connect()
        return PluginAPI::PortHandle{pi.transport};
    }

    // Buffered – use PortInfo* as handle.impl
    return PluginAPI::PortHandle{&pi};
}

// Demo transport: each port gets a tiny byte buffer in PortInfo::transport.
static std::vector<std::uint8_t> &BufferFor(PortManager::PortInfo &pi) {
    if (!pi.transport)
        pi.transport = new std::vector<std::uint8_t>(1024);
    return *static_cast<std::vector<std::uint8_t> *>(pi.transport);
}

bool PortManager::Read(PluginAPI::PortHandle h,
    void                                    *dst,
    size_t                                   bytes,
    size_t                                  &outBytes) {
    outBytes = 0;
    if (!h.impl)
        return false;

    auto *pi = static_cast<PortInfo *>(h.impl);

    if (pi->desc.AccessPolicy == PluginAPI::DataAccessPolicy::Direct) {
        return false; // Direct ports should use directPtr_, not Read()
    }

    // Find the connection where this port is the receiver
    for (auto &conn : connections_) {
        if (conn.receiver.addon == pi->key.addon &&
            conn.receiver.port == pi->key.port) {
            if (!conn.hasData)
                return false; // nothing written yet

            const size_t n = std::min(bytes, conn.buffer.size());
            std::memcpy(dst, conn.buffer.data(), n);
            outBytes = n;
            return true;
        }
    }

    return false; // no connection found
}

bool PortManager::Write(PluginAPI::PortHandle h, const void *src, size_t bytes, size_t &outBytes) {
    outBytes = 0;
    if (!h.impl)
        return false;

    auto *pi = static_cast<PortInfo *>(h.impl);

    // For Direct ports, AddOnPort::write might bypass this; for now,
    // we assume only Buffered ports call Write().
    if (pi->desc.AccessPolicy == PluginAPI::DataAccessPolicy::Direct) {
        return false;
    }

    // Find all connections where this port is the provider
    bool any = false;
    for (auto &conn : connections_) {
        if (conn.provider.addon == pi->key.addon &&
            conn.provider.port == pi->key.port) {
            const size_t n = std::min(bytes, conn.buffer.size());
            std::memcpy(conn.buffer.data(), src, n);
            conn.hasData = true;
            outBytes     = n; // last value wins for outBytes
            any          = true;
        }
    }

    return any;
}

#if 0 // jsonv ersion

// Project functions
    #include <fstream>
    #include <nlohmann/json.hpp>

using json = nlohmann::json;

bool PortManager::SaveToFile(const std::string &filename) const {
    json j;

    // Ports
    for (const auto &[key, info] : ports_) {
        const auto &d = info.desc;

        j["ports"].push_back({{"addon", key.addon},
            {"port", key.port},
            {"direction", to_string(d.Direction)},
            {"type", to_string(d.Type)},
            {"policy", to_string(d.AccessPolicy)},
            {"payload_size", d.PayloadSize},
            {"payload_hash", d.TypeHash}});
    }

    // Connections
    for (const auto &c : connections_) {
        j["connections"].push_back({{"provider_addon", c.provider.addon},
            {"provider_port", c.provider.port},
            {"receiver_addon", c.receiver.addon},
            {"receiver_port", c.receiver.port}});
    }

    std::ofstream f(filename);
    if (!f)
        return false;

    f << j.dump(4);
    return true;
}

bool PortManager::LoadFromFile(const std::string &filename) {
    std::ifstream f(filename);
    if (!f)
        return false;

    json j;
    f >> j;

    ports_.clear();
    connections_.clear();

    // Load ports
    for (const auto &entry : j["ports"]) {
        PortKey key{
            entry["addon"].get<std::string>(),
            entry["port"].get<std::string>()};

        PortDescriptor desc;
        desc.Name         = entry["port"];
        desc.Direction    = (entry["direction"] == "Input")
                                ? PortDirection::Input
                                : PortDirection::Output;
        desc.Type         = (entry["type"] == "SharedMemory")
                                ? PortType::SharedMemory
                                : (entry["type"] == "Function"
                                          ? PortType::Function
                                          : PortType::Socket);
        desc.AccessPolicy = (entry["policy"] == "Direct")
                                ? DataAccessPolicy::Direct
                                : DataAccessPolicy::Buffered;
        desc.PayloadSize  = entry["payload_size"];
        desc.TypeHash     = entry["payload_hash"];

        PortInfo info;
        info.key       = key;
        info.desc      = desc;
        info.transport = nullptr; // re-created later on connect

        ports_.emplace(key, info);
    }

    // Load connections
    for (const auto &entry : j["connections"]) {
        PortKey prov{entry["provider_addon"], entry["provider_port"]};
        PortKey recv{entry["receiver_addon"], entry["receiver_port"]};
        connections_.push_back({prov, recv});
    }

    return true;
}
#else
    #include <fstream>
    #include <limits>

bool PortManager::SaveToFile(const std::string &filename) const {
    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (!out) {
        std::cerr << "[PortManager] Failed to open file for writing: "
                  << filename << "\n";
        return false;
    }

    // Magic + version
    out << "PMv1\n";

    // Counts
    out << ports_.size() << " " << connections_.size() << "\n";

    // Ports
    for (const auto &[key, info] : ports_) {
        const auto &d = info.desc;

        out << key.addon << "\n";
        out << key.port << "\n";

        // cast enums to integer
        out << static_cast<int>(d.Direction) << " "
            << static_cast<int>(d.Type) << " "
            << static_cast<int>(d.AccessPolicy) << " "
            << d.PayloadSize << " "
            << d.TypeHash << "\n";
    }

    // Connections
    for (const auto &c : connections_) {
        out << c.provider.addon << "\n";
        out << c.provider.port << "\n";
        out << c.receiver.addon << "\n";
        out << c.receiver.port << "\n";
    }

    return true;
}
bool PortManager::LoadFromFile(const std::string &filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "[PortManager] Failed to open file for reading: "
                  << filename << "\n";
        return false;
    }

    std::string magic;
    if (!std::getline(in, magic)) {
        std::cerr << "[PortManager] Failed to read magic from file\n";
        return false;
    }
    if (magic != "PMv1") {
        std::cerr << "[PortManager] Unsupported file format: " << magic << "\n";
        return false;
    }

    std::size_t numPorts = 0;
    std::size_t numConns = 0;
    if (!(in >> numPorts >> numConns)) {
        std::cerr << "[PortManager] Failed to read counts\n";
        return false;
    }

    // consume rest of line after numbers

    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    ports_.clear();
    connections_.clear();

    // ---- Load ports ----
    for (std::size_t i = 0; i < numPorts; ++i) {
        PortKey key;
        if (!std::getline(in, key.addon)) {
            std::cerr << "[PortManager] Failed to read addon name for port "
                      << i << "\n";
            return false;
        }
        if (!std::getline(in, key.port)) {
            std::cerr << "[PortManager] Failed to read port name for port "
                      << i << "\n";
            return false;
        }

        int           dirInt = 0, typeInt = 0, polInt = 0;
        std::size_t   payloadSize = 0;
        std::uint64_t typeHash    = 0;

        if (!(in >> dirInt >> typeInt >> polInt >> payloadSize >> typeHash)) {
            std::cerr << "[PortManager] Failed to read descriptor fields for port "
                      << key.addon << "::" << key.port << "\n";
            return false;
        }
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        PluginAPI::PortDescriptor desc;
        desc.Name         = key.port;
        desc.Direction    = static_cast<PluginAPI::PortDirection>(dirInt);
        desc.Type         = static_cast<PluginAPI::PortType>(typeInt);
        desc.AccessPolicy = static_cast<PluginAPI::DataAccessPolicy>(polInt);
        desc.PayloadSize  = payloadSize;
        desc.TypeHash     = typeHash;

        PortInfo info;
        info.key       = key;
        info.desc      = desc;
        info.transport = nullptr; // will be recreated on Connect/OpenPort

        ports_.emplace(key, std::move(info));
    }

    // ---- Load connections ----
    for (std::size_t i = 0; i < numConns; ++i) {
        Connection c;
        if (!std::getline(in, c.provider.addon)) {
            std::cerr << "[PortManager] Failed to read provider addon for connection "
                      << i << "\n";
            return false;
        }
        if (!std::getline(in, c.provider.port)) {
            std::cerr << "[PortManager] Failed to read provider port for connection "
                      << i << "\n";
            return false;
        }
        if (!std::getline(in, c.receiver.addon)) {
            std::cerr << "[PortManager] Failed to read receiver addon for connection "
                      << i << "\n";
            return false;
        }
        if (!std::getline(in, c.receiver.port)) {
            std::cerr << "[PortManager] Failed to read receiver port for connection "
                      << i << "\n";
            return false;
        }

        connections_.push_back(std::move(c));
    }

    return true;
}

#endif