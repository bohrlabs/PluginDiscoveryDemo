#include <iostream>
#include "../include/PluginAPI.hpp"

// Platform-specific headers
#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <dlfcn.h>
#else
    #error Platform not supported
#endif

// Dummy service class
struct DummyServices {
    void CreatePort(const PluginAPI::PortDescriptor& desc) {
        std::cout << "Creating Port: " << desc.Name
                  << " | Direction=" << static_cast<int>(desc.Direction)
                  << " | Type=" << static_cast<int>(desc.Type)
                  << " | Policy=" << static_cast<int>(desc.AccessPolicy)
                  << "\n";
    }
};

using GetPortsFunc = std::vector<PluginAPI::PortDescriptor>(*)();

int main() {
    GetPortsFunc getPorts = nullptr;

#if defined(_WIN32)
    // ---------------- Windows DLL loading ----------------
    HMODULE mod = LoadLibraryA("MyAddon.dll");
    if (!mod) {
        std::cerr << "❌ Failed to load MyAddon.dll\n";
        return 1;
    }

    getPorts = (GetPortsFunc)GetProcAddress(mod, "GetPortDescriptors");
    if (!getPorts) {
        std::cerr << "❌ GetPortDescriptors not found\n";
        FreeLibrary(mod);
        return 1;
    }

#elif defined(__linux__)
    // ---------------- Linux .so loading ----------------
    void* handle = dlopen("./libMyAddon.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "❌ Failed to load libMyAddon.so: " << dlerror() << "\n";
        return 1;
    }

    dlerror(); // clear previous errors

    getPorts = (GetPortsFunc)dlsym(handle, "GetPortDescriptors");
    const char* err = dlerror();
    if (err) {
        std::cerr << "❌ GetPortDescriptors not found: " << err << "\n";
        dlclose(handle);
        return 1;
    }
#endif

    // ---------------- Common Code ----------------
    auto ports = getPorts();
    DummyServices svc;
    for (auto& port : ports) {
        svc.CreatePort(port);
    }

#if defined(_WIN32)
    FreeLibrary(mod);
#elif defined(__linux__)
    dlclose(handle);
#endif

    return 0;
}