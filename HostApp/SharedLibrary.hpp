#pragma once
#include <filesystem>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

class SharedLibrary {
public:
    SharedLibrary() = default;
    explicit SharedLibrary(const std::filesystem::path& p) { open(p); }

    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator=(const SharedLibrary&) = delete;

    SharedLibrary(SharedLibrary&& other) noexcept { moveFrom(other); }
    SharedLibrary& operator=(SharedLibrary&& other) noexcept {
        if (this != &other) {
            close();
            moveFrom(other);
        }
        return *this;
    }

    ~SharedLibrary() { close(); }

    bool open(const std::filesystem::path& p) {
        close();
        path_ = p;

#ifdef _WIN32
        handle_ = LoadLibraryA(p.string().c_str());
#else
        handle_ = dlopen(p.string().c_str(), RTLD_NOW);
#endif
        return handle_ != nullptr;
    }

    void close() {
        if (!handle_) return;
#ifdef _WIN32
        FreeLibrary((HMODULE)handle_);
#else
        dlclose(handle_);
#endif
        handle_ = nullptr;
    }

    bool isOpen() const { return handle_ != nullptr; }
    const std::filesystem::path& path() const { return path_; }

    std::string lastError() const {
#ifdef _WIN32
        DWORD err = GetLastError();
        if (err == 0) return {};
        LPSTR buf = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       nullptr, err, 0, (LPSTR)&buf, 0, nullptr);
        std::string msg = buf ? buf : "";
        if (buf) LocalFree(buf);
        return msg;
#else
        const char* e = dlerror();
        return e ? e : "";
#endif
    }

    template <typename Fn>
    Fn getSymbol(const char* name) const {
        if (!handle_) return nullptr;
#ifdef _WIN32
        return reinterpret_cast<Fn>(GetProcAddress((HMODULE)handle_, name));
#else
        return reinterpret_cast<Fn>(dlsym(handle_, name));
#endif
    }

private:
    void moveFrom(SharedLibrary& other) {
        handle_ = other.handle_;
        path_   = std::move(other.path_);
        other.handle_ = nullptr;
    }

    void* handle_ = nullptr;
    std::filesystem::path path_;
};
