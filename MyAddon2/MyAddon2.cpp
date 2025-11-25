#include "MyAddon2.hpp"
#include <iostream>

std::vector<PluginAPI::PortDescriptor> MyAddon2::getPortDescriptors() const {
    return {InPort, OutPort};
}

void MyAddon2::initialize(PluginAPI::IHostServices *svc) {
    InPort.Bind(svc);
    OutPort.Bind(svc);
    
}

void MyAddon2::run() {
    Packet p;

    if (InPort.read(p)) {
        std::cout << "[MyAddon2] Received: value=" << p.value
                  << " speed=" << p.speed << "\n";

        // process
        p.value *= 2;
        p.speed *= 0.5f;

        // write processed
        //OutPort.write(p);

        std::cout << "[MyAddon2] Sent Processed: value=" << p.value
                  << " speed=" << p.speed << "\n";
    } else {
        std::cout << "[MyAddon2] No input yet...\n";
    }
}

void MyAddon2::shutdown() { 
    std::cout << "[MyAddon2] shutdown\n";
}

#ifdef _WIN32
extern "C" __declspec(dllexport) PluginAPI::IPlugin *CreatePlugin() {
    return new MyAddon2();
}
extern "C" __declspec(dllexport) void DestroyPlugin(PluginAPI::IPlugin *p) {
    delete p;
}
#else
extern "C" __attribute__((visibility("default"))) PluginAPI::IPlugin *CreatePlugin() {
    return new MyAddon2();
}
extern "C" __attribute__((visibility("default"))) void DestroyPlugin(PluginAPI::IPlugin *p) {
    delete p;
}
#endif
