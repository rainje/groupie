#pragma once

// Minimal COM smart pointer - no ATL dependency
template<typename T>
class ComPtr {
public:
    ComPtr() noexcept : ptr_(nullptr) {}
    ~ComPtr() { Release(); }

    ComPtr(const ComPtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_) ptr_->AddRef();
    }
    ComPtr& operator=(const ComPtr& other) noexcept {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            if (ptr_) ptr_->AddRef();
        }
        return *this;
    }
    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T* Get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    T** GetAddressOf() noexcept { return &ptr_; }
    T** ReleaseAndGetAddressOf() noexcept { Release(); return &ptr_; }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    void Release() noexcept {
        if (ptr_) { ptr_->Release(); ptr_ = nullptr; }
    }

    template<typename U>
    HRESULT As(ComPtr<U>& other) const noexcept {
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(other.ReleaseAndGetAddressOf()));
    }

private:
    T* ptr_;
};
