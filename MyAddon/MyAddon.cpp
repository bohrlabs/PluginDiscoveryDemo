#include "MyAddon.hpp"
#include <iostream>

std::vector<PluginAPI::PortDescriptor> MyAddon::getPortDescriptors() const {
    return {OutPort};
}

void MyAddon::initialize(PluginAPI::IHostServices *svc) {
    OutPort.Bind(svc);
}

void MyAddon::run() {
    static Packet p;
    p.value = tick++;
    p.speed = 3.14f * p.value;

    // direct write into SHM
    OutPort.data() = p;

    std::cout << "[MyAddon] Produced Packet: value=" << p.value
              << " speed=" << p.speed << "\n";
}

void MyAddon::shutdown() {
    std::cout << "[MyAddon] shutdown\n";
}

#ifdef _WIN32
extern "C" __declspec(dllexport) PluginAPI::IPlugin *CreatePlugin() {
    return new MyAddon();
}
extern "C" __declspec(dllexport) void DestroyPlugin(PluginAPI::IPlugin *p) {
    delete p;
}
#else
extern "C" __attribute__((visibility("default"))) PluginAPI::IPlugin *CreatePlugin() {
    return new MyAddon();
}
extern "C" __attribute__((visibility("default"))) void DestroyPlugin(PluginAPI::IPlugin *p) {
    delete p;
}
#endif
