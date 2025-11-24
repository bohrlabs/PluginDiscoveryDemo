#include "../include/PluginAPI.hpp"

class MyAddon : public PluginAPI::IPlugin {
public:
    using InPort = PluginAPI::ConstexprPortDescriptor<
        "InputSharedMemory",
        PluginAPI::PortDirection::Input,
        PluginAPI::PortType::SharedMemory,
        PluginAPI::DataAccessPolicy::Buffered
    >;

    using OutPort = PluginAPI::ConstexprPortDescriptor<
        "OutputSharedMemory",
        PluginAPI::PortDirection::Output,
        PluginAPI::PortType::SharedMemory,
        PluginAPI::DataAccessPolicy::Direct
    >;

    std::vector<PluginAPI::PortDescriptor> getPortDescriptors() const override {
        return {
            PluginAPI::to_runtime_descriptor(InPort{}),
            PluginAPI::to_runtime_descriptor(OutPort{})
        };
    }

    void initialize() override {
        // Startup logic
    }

    void run() override {
        // Main work loop here
    }

    void shutdown() override {
        // Cleanup logic
    }
};

// Cross-platform export macro
#if defined(_WIN32)
    #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define PLUGIN_EXPORT extern "C"
#endif

PLUGIN_EXPORT PluginAPI::IPlugin* CreatePlugin() {
    return new MyAddon();
}

PLUGIN_EXPORT void DestroyPlugin(PluginAPI::IPlugin* plugin) {
    delete plugin;
}