#pragma once
#include <SDL3/SDL_gpu.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <utility>

namespace Astral {

/**
 * @brief RAII wrapper for SDL GPU resources.
 * 
 * Manages ownership of SDL GPU resources (textures, buffers, shaders, pipelines, samplers).
 * Automatically releases resources in destructor.
 * 
 * Non-copyable, move-only semantics.
 * SDL_GPUDevice is NOT owned by this wrapper (passed by reference).
 */
template<typename T>
struct GpuResource {
    T* ptr = nullptr;
    SDL_GPUDevice* device = nullptr;

    GpuResource() = default;
    
    GpuResource(T* p, SDL_GPUDevice* d) : ptr(p), device(d) {}

    // Copy semantics - shallow copy (pointer'lar paylaşılır, release etmez)
    GpuResource(const GpuResource& o) noexcept : ptr(o.ptr), device(o.device) {}
    
    GpuResource& operator=(const GpuResource& o) noexcept {
        if (this != &o) {
            ptr = o.ptr;
            device = o.device;
        }
        return *this;
    }

    // Move semantics
    GpuResource(GpuResource&& o) noexcept : ptr(o.ptr), device(o.device) {
        o.ptr = nullptr;
        o.device = nullptr;
    }

    GpuResource& operator=(GpuResource&& o) noexcept {
        if (this != &o) {
            release();
            ptr = o.ptr;
            device = o.device;
            o.ptr = nullptr;
            o.device = nullptr;
        }
        return *this;
    }

    ~GpuResource() { release(); }

    T* get() const { return ptr; }
    T* operator->() const { return ptr; }
    operator bool() const { return ptr != nullptr; }
    operator T*() const { return ptr; }

    void reset(T* p = nullptr, SDL_GPUDevice* d = nullptr) {
        release();
        ptr = p;
        device = d;
    }

private:
    void release();  // Explicit specialization for each type
};

// ============================================================================
// SPECIALIZATIONS
// ============================================================================

template<>
inline void GpuResource<SDL_GPUTexture>::release() {
    if (ptr && device) {
        SDL_ReleaseGPUTexture(device, ptr);
    }
    ptr = nullptr;
    device = nullptr;
}

template<>
inline void GpuResource<SDL_GPUBuffer>::release() {
    if (ptr && device) {
        SDL_ReleaseGPUBuffer(device, ptr);
    }
    ptr = nullptr;
    device = nullptr;
}

template<>
inline void GpuResource<SDL_GPUShader>::release() {
    if (ptr && device) {
        SDL_ReleaseGPUShader(device, ptr);
    }
    ptr = nullptr;
    device = nullptr;
}

template<>
inline void GpuResource<SDL_GPUGraphicsPipeline>::release() {
    if (ptr && device) {
        SDL_ReleaseGPUGraphicsPipeline(device, ptr);
    }
    ptr = nullptr;
    device = nullptr;
}

template<>
inline void GpuResource<SDL_GPUSampler>::release() {
    if (ptr && device) {
        SDL_ReleaseGPUSampler(device, ptr);
    }
    ptr = nullptr;
    device = nullptr;
}

// TTF_Font specialization (no device needed)
template<>
inline void GpuResource<TTF_Font>::release() {
    if (ptr) {
        TTF_CloseFont(ptr);
    }
    ptr = nullptr;
    device = nullptr;
}

// ============================================================================
// TYPE ALIASES
// ============================================================================

using GpuTexture = GpuResource<SDL_GPUTexture>;
using GpuBuffer = GpuResource<SDL_GPUBuffer>;
using GpuShader = GpuResource<SDL_GPUShader>;
using GpuPipeline = GpuResource<SDL_GPUGraphicsPipeline>;
using GpuSampler = GpuResource<SDL_GPUSampler>;
using GpuFont = GpuResource<TTF_Font>;

} // namespace Astral
