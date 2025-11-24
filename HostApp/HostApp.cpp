#include <iostream>
#include "../include/PluginAPI.hpp"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <dlfcn.h>
#else
    #error Platform not supported
#endif

struct DummyServices {
    void CreatePort(const PluginAPI::PortDescriptor& desc) {
        std::cout << "Creating Port: " << desc.Name
                  << " | Direction=" << static_cast<int>(desc.Direction)
                  << " | Type=" << static_cast<int>(desc.Type)
                  << " | Policy=" << static_cast<int>(desc.AccessPolicy)
                  << "\n";
    }
};

using CreatePluginFunc = PluginAPI::IPlugin*(*)();
using DestroyPluginFunc = void(*)(PluginAPI::IPlugin*);

int main() {
#if defined(_WIN32)
    HMODULE mod = LoadLibraryA("MyAddon.dll");
    if (!mod) { std::cerr << "❌ Failed to load MyAddon.dll\n"; return 1; }
    auto createPlugin = (CreatePluginFunc)GetProcAddress(mod, "CreatePlugin");
    auto destroyPlugin = (DestroyPluginFunc)GetProcAddress(mod, "DestroyPlugin");
#elif defined(__linux__)
    void* mod = dlopen("./libMyAddon.so", RTLD_LAZY);
    if (!mod) { std::cerr << "❌ Failed to load libMyAddon.so: " << dlerror() << "\n"; return 1; }
    dlerror();
    auto createPlugin = (CreatePluginFunc)dlsym(mod, "CreatePlugin");
    auto destroyPlugin = (DestroyPluginFunc)dlsym(mod, "DestroyPlugin");
#endif

    if (!createPlugin || !destroyPlugin) {
        std::cerr << "❌ Plugin factory functions not found\n";
        return 1;
    }

    PluginAPI::IPlugin* plugin = createPlugin();
    plugin->initialize();

    auto ports = plugin->getPortDescriptors();
    DummyServices svc;
    for (auto& p : ports) {
        svc.CreatePort(p);
    }

    plugin->shutdown();
    destroyPlugin(plugin);

#if defined(_WIN32)
    FreeLibrary(mod);
#elif defined(__linux__)
    dlclose(mod);
#endif

    return 0;
}