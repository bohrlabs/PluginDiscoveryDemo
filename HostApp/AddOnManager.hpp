#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include "../include/PluginAPI.hpp"
#include "SharedLibrary.hpp"

class AddOnManager {
public:
    struct AddOn {
        std::filesystem::path path;
        SharedLibrary         lib;
        PluginAPI::IPlugin*   plugin = nullptr;

        using CreateFn  = PluginAPI::IPlugin* (*)();
        using DestroyFn = void (*)(PluginAPI::IPlugin*);

        CreateFn  createFn  = nullptr;
        DestroyFn destroyFn = nullptr;
    };

    AddOnManager() = default;
    ~AddOnManager(); // ensures unload

    // Configure search directories
    void addSearchDir(const std::filesystem::path& dir);
    void clearSearchDirs();

    // Discovery + load
    bool scanAndLoad();

    // Access loaded addons
    const std::vector<AddOn>& addons() const { return addons_; }
    std::vector<AddOn>& addons() { return addons_; }

    // Lifecycle helpers
    void discoverPortsForAll(class IHostPortServices& svc);
    void runAll();
    void unloadAll();

private:
    static bool isAddonFile(const std::filesystem::path& p);
    static std::vector<std::filesystem::path> collectCandidates(const std::filesystem::path& dir);

    bool loadOne(const std::filesystem::path& libPath);

    std::vector<std::filesystem::path> searchDirs_;
    std::vector<AddOn> addons_;
};

// Small interface so AddOnManager doesn’t depend on your concrete services type
// In HostApp/AddOnManager.hpp replace IHostPortServices with:

class IHostPortServices {
public:
    virtual ~IHostPortServices() = default;

    // new: optional context hook
    virtual void BeginAddon(const std::string& /*addonName*/) {}

    virtual void CreatePort(const PluginAPI::PortDescriptor& desc) = 0;
};

