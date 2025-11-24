#include "AddOnManager.hpp"
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

AddOnManager::~AddOnManager() {
    unloadAll();
}

void AddOnManager::addSearchDir(const fs::path& dir) {
    searchDirs_.push_back(dir);
}

void AddOnManager::clearSearchDirs() {
    searchDirs_.clear();
}

bool AddOnManager::isAddonFile(const fs::path& p) {
    if (!p.has_extension()) return false;
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

#ifdef _WIN32
    return ext == ".dll";
#else
    return ext == ".so";
#endif
}

std::vector<fs::path> AddOnManager::collectCandidates(const fs::path& dir) {
    std::vector<fs::path> out;
    if (!fs::exists(dir) || !fs::is_directory(dir)) return out;

    for (const auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        const auto& p = e.path();
        if (isAddonFile(p)) out.push_back(p);
    }
    return out;
}

bool AddOnManager::scanAndLoad() {
    unloadAll();

    std::vector<fs::path> candidates;
    for (const auto& dir : searchDirs_) {
        auto c = collectCandidates(dir);
        candidates.insert(candidates.end(), c.begin(), c.end());
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    if (candidates.empty()) {
        std::cerr << "[AddOnManager] No addons found.\n";
        return false;
    }

    bool anyLoaded = false;
    for (const auto& p : candidates) {
        if (loadOne(p)) anyLoaded = true;
    }

    if (!anyLoaded) {
        std::cerr << "[AddOnManager] All addon loads failed.\n";
    }
    return anyLoaded;
}

bool AddOnManager::loadOne(const fs::path& libPath) {
    AddOn a;
    a.path = libPath;

    if (!a.lib.open(libPath)) {
        std::cerr << "[AddOnManager] Failed to load " << libPath
                  << " : " << a.lib.lastError() << "\n";
        return false;
    }

    a.createFn  = a.lib.getSymbol<AddOn::CreateFn>("CreatePlugin");
    a.destroyFn = a.lib.getSymbol<AddOn::DestroyFn>("DestroyPlugin");

    if (!a.createFn || !a.destroyFn) {
        std::cerr << "[AddOnManager] Missing exports in " << libPath << "\n";
        return false;
    }

    a.plugin = a.createFn();
    if (!a.plugin) {
        std::cerr << "[AddOnManager] CreatePlugin failed for " << libPath << "\n";
        return false;
    }

    std::cout << "[AddOnManager] Loaded " << libPath.filename().string() << "\n";
    addons_.push_back(std::move(a));
    return true;
}

void AddOnManager::discoverPortsForAll(IHostPortServices& svc) {
    for (auto& a:addons_) {
        const auto addonName = a.path.stem().string(); // "MyAddon2"
        std::cout<<"[AddOnManager] Ports for "<<addonName<<"\n";

        svc.BeginAddon(addonName);

        auto ports = a.plugin->getPortDescriptors();
        for (const auto& pd:ports) {
            svc.CreatePort(pd);
        }
    }
}


void AddOnManager::runAll() {
    for (auto& a : addons_) {
        std::cout << "[AddOnManager] Running " << a.path.filename().string() << "\n";
        a.plugin->initialize();
        a.plugin->run();
        a.plugin->shutdown();
    }
}

void AddOnManager::unloadAll() {
    for (auto& a : addons_) {
        if (a.plugin && a.destroyFn) {
            a.destroyFn(a.plugin);
            a.plugin = nullptr;
        }
        a.lib.close();
    }
    addons_.clear();
}
