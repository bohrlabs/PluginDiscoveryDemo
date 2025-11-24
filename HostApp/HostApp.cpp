// HostApp/HostApp.cpp
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "../include/PluginAPI.hpp"

namespace fs = std::filesystem;

using CreatePluginFn = PluginAPI::IPlugin* (*)();
using DestroyPluginFn = void (*)(PluginAPI::IPlugin*);

struct DummyServices {
    void CreatePort(const PluginAPI::PortDescriptor& desc) {
        std::cout<<"  [HostApp] Creating port: "<<desc.Name
            <<" | Dir="<<PluginAPI::to_string(desc.Direction)
            <<" | Type="<<PluginAPI::to_string(desc.Type)
            <<" | Policy="<<PluginAPI::to_string(desc.AccessPolicy)
            <<"\n";
    }
};

struct LoadedAddon {
    fs::path                path;
#ifdef _WIN32
    HMODULE                 lib = nullptr;
#else
    void* lib = nullptr;
#endif
    CreatePluginFn          createFn = nullptr;
    DestroyPluginFn         destroyFn = nullptr;
    PluginAPI::IPlugin* plugin = nullptr;
};

static bool IsAddonFile(const fs::path& p) {
#ifdef _WIN32
    return p.extension()==".dll";
#else
    // keep it simple: .so only
    return p.extension()==".so";
#endif
}

static std::vector<fs::path> CollectAddonCandidates(const fs::path& baseDir) {
    std::vector<fs::path> out;
    if (!fs::exists(baseDir)||!fs::is_directory(baseDir)) return out;

    for (const auto& e:fs::directory_iterator(baseDir)) {
        if (!e.is_regular_file()) continue;
        const auto& p = e.path();
        if (IsAddonFile(p)) out.push_back(p);
    }
    return out;
}

static LoadedAddon LoadOneAddon(const fs::path& dllPath) {
    LoadedAddon a;
    a.path = dllPath;

#ifdef _WIN32
    a.lib = LoadLibraryA(dllPath.string().c_str());
    if (!a.lib) {
        std::cerr<<"[HostApp] Failed to load "<<dllPath<<"\n";
        return a;
    }
    a.createFn = (CreatePluginFn)GetProcAddress(a.lib, "CreatePlugin");
    a.destroyFn = (DestroyPluginFn)GetProcAddress(a.lib, "DestroyPlugin");
#else
    a.lib = dlopen(dllPath.string().c_str(), RTLD_NOW);
    if (!a.lib) {
        std::cerr<<"[HostApp] Failed to load "<<dllPath<<" : "<<dlerror()<<"\n";
        return a;
    }
    a.createFn = (CreatePluginFn)dlsym(a.lib, "CreatePlugin");
    a.destroyFn = (DestroyPluginFn)dlsym(a.lib, "DestroyPlugin");
#endif

    if (!a.createFn||!a.destroyFn) {
        std::cerr<<"[HostApp] Missing exports in "<<dllPath<<"\n";
        return a;
    }

    a.plugin = a.createFn();
    if (!a.plugin) {
        std::cerr<<"[HostApp] CreatePlugin failed for "<<dllPath<<"\n";
        return a;
    }

    return a;
}

static void UnloadOneAddon(LoadedAddon& a) {
    if (a.plugin&&a.destroyFn) {
        a.destroyFn(a.plugin);
        a.plugin = nullptr;
    }

#ifdef _WIN32
    if (a.lib) {
        FreeLibrary(a.lib);
        a.lib = nullptr;
    }
#else
    if (a.lib) {
        dlclose(a.lib);
        a.lib = nullptr;
    }
#endif
}

int main() {
    std::cout<<"[HostApp] Starting multi-addon discovery demo\n";
    DummyServices svc;

    // Search order:
    // 1) ./addons
    // 2) current dir
    // 3) ./Debug, ./Release (VS layout convenience)
    std::vector<fs::path> searchDirs = {
        fs::current_path()/"bin",
        fs::current_path(),
        fs::current_path()/"Debug",
        fs::current_path()/"Release",
        fs::current_path()/"RelWithDebInfo",
        fs::current_path()/"MinSizeRel",
    };

    std::vector<fs::path> candidates;
    for (const auto& dir:searchDirs) {
        auto c = CollectAddonCandidates(dir);
        candidates.insert(candidates.end(), c.begin(), c.end());
    }

    // De-dup paths
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    if (candidates.empty()) {
        std::cerr<<"[HostApp] No addons found. Looked in:\n";
        for (auto& d:searchDirs) std::cerr<<"  "<<d<<"\n";
        return 1;
    }

    std::cout<<"[HostApp] Found "<<candidates.size()<<" addon candidate(s)\n";

    std::vector<LoadedAddon> addons;
    addons.reserve(candidates.size());

    // Load all addons
    for (const auto& p:candidates) {
        std::cout<<"[HostApp] Loading: "<<p.filename().string()<<"\n";
        auto a = LoadOneAddon(p);
        if (!a.plugin) {
            UnloadOneAddon(a);
            continue;
        }
        std::cout<<"  [HostApp] Loaded OK\n";
        addons.push_back(std::move(a));
    }

    if (addons.empty()) {
        std::cerr<<"[HostApp] All addon loads failed.\n";
        return 1;
    }

    // Discover ports for each addon
    for (auto& a:addons) {
        std::cout<<"[HostApp] Ports for: "<<a.path.filename().string()<<"\n";
        auto ports = a.plugin->getPortDescriptors();
        for (const auto& pd:ports) {
            svc.CreatePort(pd);
        }
    }

    // Run lifecycle for each addon
    for (auto& a:addons) {
        std::cout<<"[HostApp] Running: "<<a.path.filename().string()<<"\n";
        a.plugin->initialize();
        a.plugin->run();
        a.plugin->shutdown();
    }

    // Unload all
    for (auto& a:addons) {
        std::cout<<"[HostApp] Unloading: "<<a.path.filename().string()<<"\n";
        UnloadOneAddon(a);
    }

    std::cout<<"[HostApp] Done\n";
    return 0;
}
