
# PluginDiscoveryDemo

A minimal, end-to-end example of a **C++ AddOn / Plugin Discovery System** with:

- Runtime plugin loading (DLL/.so)
- Automatic port discovery using compile-time descriptors
- Typed ports (`AddOnPort<T, …>`)
- Two data access modes: **Direct** (zero-copy shared memory) and **Buffered** (copy-based with routing)
- Host-managed port connection graph
- Full separation of host and plugin logic

This repository demonstrates how a host application can dynamically load user-provided addons and connect them through typed communication ports.

## Overview

```
HostApp/
 ├─ HostApp.cpp
 ├─ AddOnManager.hpp/.cpp
 ├─ PortManager.hpp/.cpp
include/
 └─ PluginAPI.hpp
addons/
 ├─ MyAddonProducer/
 └─ MyAddonConsumer/
```

The host:

1. Scans directories for shared libraries  
2. Loads plugins using `CreatePlugin()` / `DestroyPlugin()`  
3. Queries port descriptors from each plugin  
4. Builds a connection graph between addon ports  
5. Initializes each plugin and binds ports  
6. Runs all plugins in a loop  
7. Shuts down and unloads all addons

## Plugin API

### Port definition (inside an addon)

You define ports at compile time using:

```cpp
using OutPort = PluginAPI::AddOnPort<
    Packet,
    "ProducerOutput",
    PluginAPI::PortDirection::Output,
    PluginAPI::PortType::SharedMemory,
    PluginAPI::DataAccessPolicy::Direct
>;
```

Each port carries:

- A typed payload (`Packet`)
- A name (`"ProducerOutput"`)
- A direction (Input / Output)
- A transport type (SharedMemory / Socket / Function)
- A data access policy:
  - `Direct` → zero-copy shared memory
  - `Buffered` → copied through host routing

### Available operations inside an addon:

```cpp
Packet p = InPort.data();    // read
OutPort.data() = p;          // write
auto* ptr = InPort.data().ptr(); // direct memory pointer for Direct ports
```

## Direct vs Buffered Ports

### Direct ports  
- Zero-copy  
- Both addons share the same memory block  
- Fastest possible communication  
- Last-value only (no history)  
- Ideal for real-time state like: ego velocity, transforms, OGM slices

### Buffered ports  
- Host keeps a buffer per connection  
- Writes are copied into the buffer  
- Reads copy them out  
- Can implement queue / mailbox / event semantics  
- Ideal for message-like data or streaming

## Host: Connecting Ports

Connections between plugins are made in the host:

```cpp
portMgr.Connect(
    {"MyAddonProducer", "ProducerOutput"},
    {"MyAddonConsumer", "ConsumerInput"}
);
```

For Direct ports:

- The host allocates exactly one shared memory block.
- Both sides get a pointer to that block.
- Reads & writes operate on the same memory.

For Buffered ports:

- The host allocates a buffer per connection.
- Writes copy into the buffer, reads copy out.

## AddOn Lifecycle

Every plugin implements:

```cpp
class IPlugin {
public:
    virtual std::vector<PortDescriptor> getPortDescriptors() const = 0;
    virtual void initialize(IHostServices* services) = 0;
    virtual void run() = 0;
    virtual void shutdown() = 0;
};
```

### Execution order:

1. `getPortDescriptors()`  
2. Host connects ports  
3. `initialize(services)`  
   - Ports get bound: `AddOnPort::Bind()`  
4. `run()` is called repeatedly  
5. `shutdown()`

## Example: Producer Addon

```cpp
void MyAddonProducer::run() {
    Packet p{counter_, counter_ * 0.1f};
    counter_++;

    OutPort.data() = p;   // writes into Direct buffer
}
```

## Example: Consumer Addon

```cpp
void MyAddonConsumer::run() {
    Packet p{};
    if (InPort.read(p)) {
        std::cout << "Got packet: " << p.counter << "\n";
    }
}
```

## PortManager Responsibilities

`PortManager` acts as the host-side communication fabric:

### In `Connect()`
- Validates ports  
- Allocates shared memory for Direct ports  
- Creates routing buffers for Buffered ports  
- Stores the connection graph

### In `OpenPort()`
- Returns pointer to shared memory (Direct)
- Returns PortInfo* for buffered routing

### In `Read()/Write()`
- Direct ports ignore this  
- Buffered ports use it to copy data between addons

## Running the Demo

1. Build the project (Visual Studio / CMake)
2. Ensure addon DLLs are located in:
   - `./addons/`
   - or next to the HostApp executable
3. Run HostApp

## Adding a New Addon

Create a new DLL with:

```cpp
extern "C" __declspec(dllexport)
PluginAPI::IPlugin* CreatePlugin() { return new MyAddon(); }

extern "C" __declspec(dllexport)
void DestroyPlugin(PluginAPI::IPlugin* p) { delete p; }
```

Place it in `/addons`.

## License

MIT License
