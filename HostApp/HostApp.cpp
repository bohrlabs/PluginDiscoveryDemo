#include <iostream>
#include <filesystem>
#include "AddOnManager.hpp"
#include "PortManager.hpp"

namespace fs = std::filesystem;

int main() {
    std::cout << "[HostApp] Starting AddOnManager + PortManager demo\n";

    AddOnManager mgr;
    PortManager  portMgr;

    mgr.addSearchDir(fs::current_path() / "bin");

    if (!mgr.scanAndLoad()) {
        std::cerr << "[HostApp] No addons loaded.\n";
        return 1;
    }

    // Register all ports in PortManager
    mgr.discoverPortsForAll(portMgr);

    portMgr.Connect("MyAddon", "OutPacket",
        "MyAddon2", "InPacket");

    portMgr.Connect("MyAddon", "OutPacket",
        "MyAddon3", "InPacket");

    portMgr.PrintConnections();
    mgr.runAll(portMgr);

    mgr.unloadAll();

    std::cout << "[HostApp] Done\n";
    return 0;
}
