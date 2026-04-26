# SDL_GPU Kullanım Örneği - 3D Küp Çizimi

Bu dokümantasyon, Astral Engine'de SDL_GPU kullanarak basit bir 3D küp çizmeyi adım adım gösterir.

## 1. GPU Device Oluşturma (App Başlangıcı)

```cpp
// engine/core/app.cpp

#include <SDL3/SDL.h>

class App {
private:
    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_gpuDevice = nullptr;
    SDL_GPUTexture* m_depthTexture = nullptr;
    
public:
    bool init() {
        // SDL başlat
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL başlatılamadı: %s", SDL_GetError());
            return false;
        }

        // Window oluştur
        m_window = SDL_CreateWindow(
            "Astral Engine - SDL_GPU",
            1280, 720,
            SDL_WINDOW_VULKAN // veya SDL_WINDOW_METAL, SDL_WINDOW_DIRECT3D12
        );

        // GPU Device oluştur
        m_gpuDevice = SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_DXIL | // Windows (DirectX 12)
            SDL_GPU_SHADERFORMAT_SPIRV | // Linux/Android (Vulkan)
            SDL_GPU_SHADERFORMAT_MSL,    // macOS/iOS (Metal)
            true, // debug mode
            nullptr
        );

        if (!m_gpuDevice) {
            SDL_Log("GPU Device oluşturulamadı: %s", SDL_GetError());
            return false;
        }

        // Window'u GPU'ya claim et
        if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice, m_window)) {
            SDL_Log("Window GPU'ya claim edilemedi: %s", SDL_GetError());
            return false;
        }

        // Depth texture oluştur
        m_depthTexture = SDL_CreateGPUTexture(m_gpuDevice, &(SDL_GPUTextureCreateInfo){
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = 1280,
            .height = 720,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .props = 0
        });

        // AssetManager'a device'ı ver
        AssetManager::getInstance().setGPUDevice(m_gpuDevice);

        return true;
    }
};
```

## 2. Mesh Verisi Hazırlama

```cpp
// Küp vertex'leri oluştur
std::vector<Vertex> createCubeVertices() {
    return {
        // Front face (kırmızı)
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

        // Back face (yeşil)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

        // Top face (mavi)
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},

        // Bottom face (sarı)
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},

        // Right face (macenta)
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

        // Left face (cyan)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
}

std::vector<uint32_t> createCubeIndices() {
    return {
        0,  1,  2,   0,  2,  3,   // Front
        4,  5,  6,   4,  6,  7,   // Back
        8,  9,  10,  8,  10, 11,  // Top
        12, 13, 14,  12, 14, 15,  // Bottom
        16, 17, 18,  16, 18, 19,  // Right
        20, 21, 22,  20, 22, 23   // Left
    };
}
```

## 3. Asset Yükleme

```cpp
void loadAssets() {
    AssetManager& assetMgr = AssetManager::getInstance();

    // 1. Mesh yükle
    auto vertices = createCubeVertices();
    auto indices = createCubeIndices();
    assetMgr.uploadMesh("cube", vertices, indices, false); // static mesh

    // 2. Shader'ları yükle
    SDL_GPUShader* vertShader = loadShaderFromFile(
        m_gpuDevice, 
        "assets/shaders/basic_3d.vert.dxil", // Windows için
        SDL_GPU_SHADERSTAGE_VERTEX,
        "main", // entry point
        SDL_GPU_SHADERFORMAT_DXIL
    );

    SDL_GPUShader* fragShader = loadShaderFromFile(
        m_gpuDevice,
        "assets/shaders/basic_3d.frag.dxil",
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        "main",
        SDL_GPU_SHADERFORMAT_DXIL
    );

    // 3. Pipeline oluştur
    SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(
        m_gpuDevice, 
        m_window
    );

    assetMgr.createPipeline(
        "default",
        vertShader,
        fragShader,
        swapchainFormat,
        true, // depth test enabled
        SDL_GPU_CULLMODE_BACK
    );

    SDL_Log("Assets yüklendi!");
}

// Shader yükleme helper fonksiyonu
SDL_GPUShader* loadShaderFromFile(
    SDL_GPUDevice* device,
    const char* filepath,
    SDL_GPUShaderStage stage,
    const char* entrypoint,
    SDL_GPUShaderFormat format)
{
    // Dosyayı oku
    SDL_IOStream* file = SDL_IOFromFile(filepath, "rb");
    if (!file) {
        SDL_Log("Shader dosyası açılamadı: %s", filepath);
        return nullptr;
    }

    Sint64 fileSize = SDL_GetIOSize(file);
    void* shaderCode = SDL_malloc(fileSize);
    SDL_ReadIO(file, shaderCode, fileSize);
    SDL_CloseIO(file);

    // Shader oluştur
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &(SDL_GPUShaderCreateInfo){
        .code_size = (size_t)fileSize,
        .code = (const Uint8*)shaderCode,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1, // MVP matrisleri için
        .props = 0
    });

    SDL_free(shaderCode);
    return shader;
}
```

## 4. Entity Oluşturma

```cpp
void createCubeEntity() {
    auto cube = m_entityManager.addEntity("rotating_cube");
    
    // Mesh component
    cube->cMesh = CMesh("cube", "default");
    cube->cMesh.has = true;

    // Transform component
    cube->cTransform = CTransform(
        glm::vec3(0.0f, 0.0f, -5.0f), // position (kameradan 5 birim uzakta)
        glm::vec3(0.0f, 0.0f, 0.0f)   // velocity
    );
    cube->cTransform.scale = glm::vec3(1.0f);
    cube->cTransform.rotation = glm::vec3(0.0f);
    cube->cTransform.has = true;
}
```

## 5. Render Loop

```cpp
void render(float deltaTime) {
    // Küpü döndür
    for (auto& entity : m_entityManager.getEntities()) {
        if (entity->cTransform.has) {
            entity->cTransform.rotation.y += 1.0f * deltaTime; // Y ekseninde dön
            entity->cTransform.rotation.x += 0.5f * deltaTime; // X ekseninde dön
        }
    }

    // View ve Projection matrisleri
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),  // kamera pozisyonu
        glm::vec3(0.0f, 0.0f, -1.0f), // bakış yönü
        glm::vec3(0.0f, 1.0f, 0.0f)   // yukarı vektör
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),  // FOV
        1280.0f / 720.0f,     // aspect ratio
        0.1f,                 // near plane
        100.0f                // far plane
    );

    // Command buffer al
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);

    // Swapchain texture al
    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmd, m_window, &swapchainTexture, nullptr, nullptr)) {
        SDL_Log("Swapchain texture alınamadı!");
        return;
    }

    if (swapchainTexture) {
        // Render pass başlat
        RenderSystem::beginRenderPass(
            cmd, 
            swapchainTexture, 
            m_depthTexture,
            glm::vec4(0.1f, 0.1f, 0.15f, 1.0f) // koyu mavi clear color
        );

        // Viewport ayarla
        RenderSystem::setViewport(0, 0, 1280, 720);

        // Entity'leri çiz
        RenderSystem::update(m_entityManager, cmd, view, projection);

        // Render pass bitir
        RenderSystem::endRenderPass();
    }

    // Command buffer'ı submit et
    SDL_SubmitGPUCommandBuffer(cmd);
}
```

## 6. Ana Loop

```cpp
void run() {
    bool running = true;
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (running) {
        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Delta time hesapla
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastTime) / (float)SDL_GetPerformanceFrequency();
        lastTime = currentTime;

        // Render
        render(deltaTime);

        // FPS limiti (opsiyonel)
        SDL_Delay(1);
    }
}
```

## 7. Cleanup

```cpp
void cleanup() {
    // AssetManager cleanup (mesh, pipeline, texture)
    AssetManager::getInstance().cleanup();

    // Depth texture
    if (m_depthTexture) {
        SDL_ReleaseGPUTexture(m_gpuDevice, m_depthTexture);
    }

    // GPU Device
    if (m_gpuDevice) {
        SDL_DestroyGPUDevice(m_gpuDevice);
    }

    // Window
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
}
```

## Beklenen Sonuç

Ekranda renkli yüzlere sahip, dönen bir 3D küp görmelisiniz:
- Her yüz farklı renkte (kırmızı, yeşil, mavi, sarı, macenta, cyan)
- Küp Y ve X eksenlerinde döner
- Depth testing sayesinde doğru yüzler önde görünür
- Backface culling sayesinde arka yüzler çizilmez

## Performance Metrikleri

Bu basit örnek için beklenen performans:
- **Draw Calls**: 1 (tüm küp tek draw call'da)
- **Vertices**: 24
- **Triangles**: 12
- **FPS**: 1000+ (modern GPU'larda)

## Troubleshooting

### Ekran siyah görünüyor
- Shader'ların doğru compile edildiğinden emin olun
- Depth texture'ın doğru oluşturulduğunu kontrol edin
- RenderDoc ile frame capture alıp debug edin

### Küp görünmüyor
- Kamera pozisyonunu kontrol edin (küp -5 Z'de)
- Projection matrix'in doğru olduğunu kontrol edin
- Culling mode'u geçici olarak NONE yapıp test edin

### Crash oluyor
- GPU Device'ın doğru oluşturulduğunu kontrol edin
- Shader formatının platform ile uyumlu olduğunu kontrol edin
- Validation layers'ı aktif edip hata mesajlarını okuyun

---

**Başarılar! 🎮**
