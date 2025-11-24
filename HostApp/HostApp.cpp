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

    portMgr.PrintPorts();

    // Example: connect MyAddon2 output to MyAddon input
    portMgr.Connect("MyAddon2", "OutputSharedMemory",
                    "MyAddon",  "InputSharedMemory");

    portMgr.PrintConnections();

    mgr.runAll();
    mgr.unloadAll();

    std::cout << "[HostApp] Done\n";
    return 0;
}
