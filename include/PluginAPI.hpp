#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace PluginAPI {

    // ================================================================
    // ENUMS
    // ================================================================
    enum struct PortDirection : std::uint8_t {
        Input  = 0,
        Output = 1
    };

    enum struct PortType : std::uint8_t {
        SharedMemory   = 0,
        InternalMemory = 1,
        Socket         = 2,
        Function       = 3
    };

    enum struct DataAccessPolicy : std::uint8_t {
        Direct   = 0,
        Buffered = 1
    };

    inline const char *to_string(PortDirection d) {
        switch (d) {
        case PortDirection::Input: return "Input";
        case PortDirection::Output: return "Output";
        default: return "Unknown";
        }
    }
    inline const char *to_string(PortType t) {
        switch (t) {
        case PortType::SharedMemory: return "SharedMemory";
        case PortType::InternalMemory: return "InternalMemory";
        case PortType::Socket: return "Socket";
        case PortType::Function: return "Function";
        default: return "Unknown";
        }
    }
    inline const char *to_string(DataAccessPolicy p) {
        switch (p) {
        case DataAccessPolicy::Direct: return "Direct";
        case DataAccessPolicy::Buffered: return "Buffered";
        default: return "Unknown";
        }
    }

    // ================================================================
    // PORT DESCRIPTOR
    // ================================================================
    struct PortDescriptor {
            std::string      Name;
            PortDirection    Direction{};
            PortType         Type{};
            DataAccessPolicy AccessPolicy{};

            // Payload typing (for host-side validation / allocation)
            std::size_t   PayloadSize = 0;
            std::uint64_t TypeHash    = 0;
    };

    // ================================================================
    // fixed_string for NTTP
    // ================================================================
    template<std::size_t N>
    struct fixed_string {
            char v[N];
            constexpr fixed_string(const char (&s)[N]) {
                for (std::size_t i = 0; i < N; ++i)
                    v[i] = s[i];
            }
            constexpr const char *c_str() const {
                return v;
            }
    };

    // ================================================================
    // Type hashing for compile-time payload identity
    // ================================================================
    template<class T>
    consteval std::uint64_t TypeHashOf() {
#if defined(__clang__) || defined(__GNUC__)
        // Can use __builtin_crc32c safely
        return __builtin_crc32c(
            0,
            (const unsigned char *)__PRETTY_FUNCTION__,
            sizeof(__PRETTY_FUNCTION__) - 1);
#else
        // MSVC: no builtin CRC — use a simple constexpr FNV-1a based on __FUNCSIG__
        constexpr const char *sig  = __FUNCSIG__;
        std::uint64_t         hash = 1469598103934665603ULL; // FNV offset basis
        for (size_t i = 0; sig[i] != 0; i++) {
            hash ^= (unsigned char)sig[i];
            hash *= 1099511628211ULL; // FNV prime
        }
        return hash;
#endif
    }

    // ================================================================
    // Host→Plugin service interface
    // (Binding, read/write, transport abstraction)
    // ================================================================
    struct PortHandle {
            void *impl = nullptr; // host-defined transport pointer
    };

    class IHostServices {
        public:
            virtual ~IHostServices() = default;

            // Return transport handle for a port
            virtual PortHandle OpenPort(const char *name) = 0;

            // For buffered ports, raw byte I/O
            virtual bool Read(PortHandle h, void *dst, size_t bytes, size_t &outBytes)        = 0;
            virtual bool Write(PortHandle h, const void *src, size_t bytes, size_t &outBytes) = 0;
    };

    // ================================================================
    // DataProxy<T> - sugar for typed access
    // ================================================================
    template<class T>
    class DataProxy {
        public:
            DataProxy(T       *directPtr,
                IHostServices *svc,
                PortHandle     h,
                bool           isOutput,
                bool           isDirect)
                : ptr_(directPtr), svc_(svc), handle_(h), isOutput_(isOutput), isDirect_(isDirect) {}

            // Read as value
            operator T() const {
                if (isDirect_ && ptr_)
                    return *ptr_;

                T      tmp{};
                size_t got = 0;
                if (svc_ && svc_->Read(handle_, &tmp, sizeof(T), got) &&
                    got == sizeof(T))
                    return tmp;

                return {};
            }

            // Write via assignment
            DataProxy &operator=(const T &v) {
                if (!isOutput_)
                    return *this;

                if (isDirect_ && ptr_) {
                    *ptr_ = v;
                    return *this;
                }

                size_t wrote = 0;
                if (svc_)
                    svc_->Write(handle_, &v, sizeof(T), wrote);
                return *this;
            }

            // Pointer access for direct SHM ports
            T *ptr() {
                return ptr_;
            }
            const T *ptr() const {
                return ptr_;
            }

        private:
            T             *ptr_ = nullptr;
            IHostServices *svc_ = nullptr;
            PortHandle     handle_{};
            bool           isOutput_ = false;
            bool           isDirect_ = false;
    };

    // ================================================================
    // AddOnPort<T, …> - typed member ports
    // ================================================================
    template<
        class PayloadT,
        fixed_string     Name,
        PortDirection    Direction,
        PortType         Type,
        DataAccessPolicy AccessPolicy>
    class AddOnPort {
        public:
            using T = PayloadT;

            static constexpr auto name         = Name;
            static constexpr auto direction    = Direction;
            static constexpr auto type         = Type;
            static constexpr auto accessPolicy = AccessPolicy;

            AddOnPort() = default;

            // -------- Descriptor for discovery --------
            operator PortDescriptor() const {
                return PortDescriptor{
                    name.c_str(),
                    direction,
                    type,
                    accessPolicy,
                    sizeof(T),
                    TypeHashOf<T>()};
            }

            // -------- Binding from host --------
            void Bind(IHostServices *svc) {
                svc_ = svc;
                if (svc) {
                    handle_ = svc->OpenPort(name.c_str());
                } else {
                    handle_ = PortHandle{};
                }
                //handle_ = svc ? svc->OpenPort(name.c_str()) : PortHandle{};

                if (accessPolicy == DataAccessPolicy::Direct &&
                    handle_.impl != nullptr) {
                    directPtr_ = static_cast<T *>(handle_.impl);
                }
            }

            // -------- Typed data access --------
            DataProxy<T> data() {
                return DataProxy<T>(
                    directPtr_,
                    svc_,
                    handle_,
                    Direction == PortDirection::Output,
                    AccessPolicy == DataAccessPolicy::Direct);
            }

            DataProxy<const T> data() const {
                return DataProxy<const T>(
                    directPtr_,
                    svc_,
                    handle_,
                    false,
                    AccessPolicy == DataAccessPolicy::Direct);
            }

            // Explicit read/write
            bool read(T &out) const {
                if (accessPolicy == DataAccessPolicy::Direct && directPtr_) {
                    out = *directPtr_;
                    return true;
                }
                size_t got = 0;
                return svc_ &&
                       svc_->Read(handle_, &out, sizeof(T), got) &&
                       got == sizeof(T);
            }

            bool write(const T &v) {
                if (accessPolicy == DataAccessPolicy::Direct && directPtr_) {
                    *directPtr_ = v;
                    return true;
                }
                size_t wrote = 0;
                return svc_ &&
                       svc_->Write(handle_, &v, sizeof(T), wrote) &&
                       wrote == sizeof(T);
            }

        private:
            IHostServices *svc_ = nullptr;
            PortHandle     handle_{};
            T             *directPtr_ = nullptr;
    };

    // ================================================================
    // IPlugin
    // ================================================================
    class IPlugin {
        public:
            virtual ~IPlugin() = default;

            virtual std::vector<PortDescriptor> getPortDescriptors() const = 0;

            // Called once host created transports + connections
            virtual void initialize(IHostServices *services) = 0;

            virtual void run()      = 0;
            virtual void shutdown() = 0;
    };

} // namespace PluginAPI
