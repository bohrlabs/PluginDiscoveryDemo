#include "MyAddon.hpp"
#include <iostream>

MyAddon::MyAddon() = default;

std::vector<PluginAPI::PortDescriptor> MyAddon::getPortDescriptors() const {
    // Implicit conversion from AddOnPort -> PortDescriptor.
    return { InPort, OutPort, InPort2 };
}

void MyAddon::initialize() {
    std::cout<<"[MyAddon] initialize()\n";
    // Example: attach runtime data/handles if needed:
    // InPort.Data = ...;
    // OutPort.Data = ...;
}

void MyAddon::run() {
    std::cout<<"[MyAddon] run()\n";
    // Example usage:
    // if (InPort.Data) { ... }
}

void MyAddon::shutdown() {
    std::cout<<"[MyAddon] shutdown()\n";
}

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

PLUGIN_EXPORT PluginAPI::IPlugin* CreatePlugin() {
    return new MyAddon();
}

PLUGIN_EXPORT void DestroyPlugin(PluginAPI::IPlugin* p) {
    delete p;
}
