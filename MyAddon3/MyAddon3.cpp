#include "MyAddon3.hpp"
#include <iostream>

std::vector<PluginAPI::PortDescriptor> MyAddon3::getPortDescriptors() const {
    return {InPort, OutPort};
}

void MyAddon3::initialize(PluginAPI::IHostServices *svc) {
    InPort.Bind(svc);
    OutPort.Bind(svc);
    
}

void MyAddon3::run() {
    Packet p;

    if (InPort.read(p)) {
        std::cout << "[MyAddon3] Received: value=" << p.value
                  << " speed=" << p.speed << "\n";

        // process
        p.value *= 2;
        p.speed *= 0.5f;

        // write processed
        //OutPort.write(p);

        std::cout << "[MyAddon3] Sent Processed: value=" << p.value
                  << " speed=" << p.speed << "\n";
    } else {
        std::cout << "[MyAddon3] No input yet...\n";
    }
}

void MyAddon3::shutdown() { 
    std::cout << "[MyAddon3] shutdown\n";
}

#ifdef _WIN32
extern "C" __declspec(dllexport) PluginAPI::IPlugin *CreatePlugin() {
    return new MyAddon3();
}
extern "C" __declspec(dllexport) void DestroyPlugin(PluginAPI::IPlugin *p) {
    delete p;
}
#else
extern "C" __attribute__((visibility("default"))) PluginAPI::IPlugin *CreatePlugin() {
    return new MyAddon3();
}
extern "C" __attribute__((visibility("default"))) void DestroyPlugin(PluginAPI::IPlugin *p) {
    delete p;
}
#endif
