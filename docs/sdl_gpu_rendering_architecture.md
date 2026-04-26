# SDL_GPU 3D Rendering Mimarisi - Astral Engine

## 📋 Genel Bakış

Bu dokümantasyon, Astral Engine'in SDL 3.4.4'ün modern **SDL_GPU API**'si ile kurulmuş 3D rendering mimarisini açıklar. Mimari, AAA oyun motorlarındaki (Vulkan/DirectX12/Metal) best practice'lere uygun olarak tasarlanmıştır.

## 🎯 Temel Prensipler

### 1. Data-Oriented Design (DOD)
- ECS (Entity-Component-System) tabanlı mimari
- Cache-friendly veri yapıları
- Minimal state changes

### 2. Explicit Resource Management
- Transfer Buffer sistemi ile staging
- Cycling mekanizması ile frame-to-frame dependency yönetimi
- Pipeline ve buffer caching

### 3. Modern Graphics API Workflow
- Command Buffer tabanlı rendering
- Render Pass sistemi
- Push Constants (Uniform Data)

---

## 🏗️ Mimari Bileşenler

### 1. Vertex Layout (`engine/math/vertex.h`)

```cpp
struct Vertex {
    glm::vec3 position;  // 12 bytes
    float _padding1;     // 4 bytes (GLSL std140 alignment)
    glm::vec3 color;     // 12 bytes
    float _padding2;     // 4 bytes
    glm::vec2 uv;        // 8 bytes
    glm::vec2 _padding3; // 8 bytes
    // Toplam: 48 bytes (16'nın katı)
};
```

**Neden Padding?**
- GLSL std140 layout kuralları gereği `vec3` sonrası 16-byte alignment gerekir
- GPU memory access performansı için kritik
- Shader'daki layout ile CPU'daki struct birebir eşleşmelidir

**Vertex Attributes:**
- Location 0: Position (vec3)
- Location 1: Color (vec3)
- Location 2: UV (vec2)

---

### 2. Resource Upload - Staging Buffer Sistemi

#### Problem: CPU → GPU Veri Transferi

CPU'daki `std::vector<Vertex>` verisini GPU'nun VRAM'ine aktarmak için **Transfer Buffer** (staging buffer) kullanılır.

#### Workflow:

```cpp
// 1. VRAM'de hedef buffer oluştur
SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(device, {
    .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
    .size = vertices.size() * sizeof(Vertex)
});

// 2. CPU tarafında Transfer Buffer oluştur
SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = vertices.size() * sizeof(Vertex)
});

// 3. Transfer Buffer'a veriyi yaz (CPU timeline)
void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
SDL_UnmapGPUTransferBuffer(device, transferBuffer);

// 4. Copy Pass ile GPU'ya transfer et (GPU timeline)
SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

SDL_UploadToGPUBuffer(copyPass, 
    &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transferBuffer, .offset = 0 },
    &(SDL_GPUBufferRegion){ .buffer = vertexBuffer, .offset = 0, .size = ... },
    false // cycle
);

SDL_EndGPUCopyPass(copyPass);
SDL_SubmitGPUCommandBuffer(cmd);

// 5. Transfer buffer'ı temizle
SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
```

**Önemli Notlar:**
- Transfer Buffer CPU tarafında map edilebilir (host-visible memory)
- GPU Buffer VRAM'de yaşar (device-local memory)
- Copy Pass, GPU timeline'da çalışır (asenkron)
- Memory barrier'lar SDL_GPU tarafından otomatik eklenir

---

### 3. Cycling Mekanizması

#### Problem: Frame-to-Frame Data Dependencies

```cpp
// Frame 1
SDL_UploadToGPUBuffer(..., myBuffer, ...);
SDL_DrawGPUIndexedPrimitives(...); // myBuffer kullanıyor

// Frame 2 - PROBLEM!
SDL_UploadToGPUBuffer(..., myBuffer, ...); // Frame 1 henüz bitmedi!
```

#### Çözüm: Cycling

SDL_GPU, her buffer/texture'ı **ring buffer** olarak yönetir:

```cpp
// Internal structure (simplified)
struct MetalBufferContainer {
    MetalBuffer* activeBuffer;
    MetalBuffer** buffers; // Ring buffer
    uint32_t bufferCount;
};
```

**Cycle = true** olduğunda:
1. Eğer active buffer bound ise → sonraki unbound buffer'a geç
2. Tüm buffer'lar bound ise → yeni buffer oluştur
3. Data integrity korunur, frame dependency kırılır

**Ne Zaman Cycle Kullanılır?**
- ✅ Her frame güncellenen dynamic buffer'lar
- ✅ Render target olarak kullanılan texture'lar (frame dependency kırmak için)
- ✅ Transfer buffer'lara yeni veri yazarken
- ❌ Static mesh'ler (bir kez yüklenip hiç değişmeyenler)
- ❌ Mevcut içeriği korumak istediğinizde

```cpp
// Örnek: Dynamic vertex buffer (her frame güncellenir)
void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, true); // CYCLE!
// ... veri yaz ...
SDL_UnmapGPUTransferBuffer(device, transferBuffer);

SDL_UploadToGPUBuffer(copyPass, ..., true); // CYCLE!
```

---

### 4. Pipeline Management - Caching

#### Pipeline Nedir?

Pipeline, GPU'nun rendering state'ini tanımlar:
- Vertex/Fragment shader'lar
- Vertex input layout
- Rasterizer state (cull mode, fill mode)
- Depth/stencil state
- Blend state
- Render target formatları

**Pipeline oluşturma pahalıdır!** Bu yüzden cache'lenir:

```cpp
// AssetManager içinde
std::map<std::string, SDL_GPUGraphicsPipeline*> m_pipelines;

SDL_GPUGraphicsPipeline* getPipeline(const std::string& name) {
    auto it = m_pipelines.find(name);
    return (it != m_pipelines.end()) ? it->second : nullptr;
}
```

**Best Practice:**
- Pipeline'ları uygulama başlangıcında oluştur
- Runtime'da pipeline oluşturmaktan kaçın
- Material sistemi için pipeline varyantları kullan

---

### 5. Uniform Data - MVP Matrisleri

#### Push Uniform Data (Push Constants Benzeri)

SDL_GPU, küçük uniform verilerini command buffer'a direkt yazar:

```cpp
struct UniformData {
    glm::mat4 model;      // 64 bytes
    glm::mat4 view;       // 64 bytes
    glm::mat4 projection; // 64 bytes
    // Toplam: 192 bytes
};

UniformData uniforms = { ... };

SDL_PushGPUVertexUniformData(
    commandBuffer,
    0, // slot index (shader'da binding = 0)
    &uniforms,
    sizeof(UniformData)
);
```

**Shader Tarafı (HLSL):**
```hlsl
cbuffer UniformBlock : register(b0, space1) // space1 = vertex uniform
{
    float4x4 model      : packoffset(c0);
    float4x4 view       : packoffset(c4);
    float4x4 projection : packoffset(c8);
};
```

**Önemli:**
- ✅ Küçük veriler için (1-2 matrix): Push Uniform Data
- ❌ Büyük veriler için (light arrays, material arrays): Storage Buffer kullan
- Vulkan spec minimum 128 bytes garanti eder, SDL_GPU daha fazlasını destekler
- Her frame değişen veriler için idealdir

**Storage Buffer Örneği (büyük veriler için):**
```cpp
// Light array gibi büyük veriler
SDL_BindGPUVertexStorageBuffers(renderPass, 0, &lightBuffer, 1);
```

---

### 6. State Change Minimization

#### Problem: Gereksiz Bind İşlemleri

```cpp
// KÖTÜ - Her draw'da pipeline bind
for (auto& entity : entities) {
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline); // WASTE!
    SDL_BindGPUVertexBuffers(...);
    SDL_DrawGPUIndexedPrimitives(...);
}
```

#### Çözüm: State Tracking

```cpp
// İYİ - Sadece değişenleri bind et
SDL_GPUGraphicsPipeline* lastPipeline = nullptr;
std::string lastMeshName = "";

for (auto& entity : entities) {
    // Pipeline değiştiyse bind et
    if (pipeline != lastPipeline) {
        SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
        lastPipeline = pipeline;
    }

    // Mesh değiştiyse buffer'ları bind et
    if (entity->meshName != lastMeshName) {
        SDL_BindGPUVertexBuffers(...);
        SDL_BindGPUIndexBuffer(...);
        lastMeshName = entity->meshName;
    }

    // Uniform data her entity için farklı (MVP)
    SDL_PushGPUVertexUniformData(...);

    // Draw
    SDL_DrawGPUIndexedPrimitives(...);
}
```

**Best Practices:**
- Material'lere göre sırala (pipeline değişimini minimize et)
- Mesh'lere göre sırala (buffer değişimini minimize et)
- Instancing kullan (aynı mesh'i çok kez çizmek için)

---

## 🎨 Render Loop Örneği

```cpp
// 1. Command Buffer al
SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);

// 2. Swapchain texture al (window'a çizmek için)
SDL_GPUTexture* swapchainTexture = nullptr;
SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTexture, nullptr, nullptr);

// 3. Render Pass başlat
RenderSystem::beginRenderPass(cmd, swapchainTexture, depthTexture, clearColor);

// 4. Viewport ayarla
RenderSystem::setViewport(0, 0, windowWidth, windowHeight);

// 5. Entity'leri çiz
RenderSystem::update(entityManager, cmd, viewMatrix, projectionMatrix);

// 6. Render Pass bitir
RenderSystem::endRenderPass();

// 7. Command Buffer'ı submit et
SDL_SubmitGPUCommandBuffer(cmd);
```

---

## 📊 Performance Tips

### 1. Render Pass Optimization
- ✅ Mümkün olduğunca az render pass kullan
- ✅ Render pass içinde birden fazla draw call yap
- ❌ Her draw için yeni render pass açma

### 2. Data Upload
- ✅ Veri yüklemelerini frame'in başında yap
- ✅ Static mesh'leri uygulama başlangıcında yükle
- ❌ Render pass sırasında veri yükleme

### 3. Resource Management
- ✅ Resource'ları cache'le (pipeline, buffer, texture)
- ✅ Resource churn'den kaçın (sürekli create/destroy)
- ❌ Her frame yeni resource oluşturma

### 4. Culling
- ✅ Frustum culling uygula (görünmeyen objeleri çizme)
- ✅ Occlusion culling (kapalı objeleri çizme)
- ✅ Backface culling (arka yüzleri çizme)

### 5. Batching
- ✅ Aynı material'li objeleri grupla
- ✅ Instancing kullan (aynı mesh, farklı transform)
- ✅ Sprite batching (2D için)

---

## 🔧 Shader Binding Layout (SDL_GPU Kuralları)

### Vertex Shader
- **Uniform Buffer**: `register(b0, space1)` → Slot 0
- **Storage Buffer**: `register(t0, space0)` → Slot 0
- **Sampler**: `register(s0, space0)` → Slot 0

### Fragment Shader
- **Uniform Buffer**: `register(b0, space2)` → Slot 0
- **Storage Buffer**: `register(t0, space3)` → Slot 0
- **Sampler**: `register(s0, space2)` → Slot 0

**Örnek:**
```hlsl
// Vertex Uniform
cbuffer VertexUniforms : register(b0, space1) { ... };

// Fragment Sampler
Texture2D<float4> mainTexture : register(t0, space2);
SamplerState mainSampler : register(s0, space2);
```

---

## 🚀 Kullanım Örneği

```cpp
// 1. GPU Device oluştur
SDL_GPUDevice* device = SDL_CreateGPUDevice(
    SDL_GPU_SHADERFORMAT_DXIL, // Windows için
    true, // debug mode
    nullptr
);

// 2. AssetManager'a device'ı ver
AssetManager::getInstance().setGPUDevice(device);

// 3. Mesh yükle
std::vector<Vertex> vertices = { ... };
std::vector<uint32_t> indices = { ... };
AssetManager::getInstance().uploadMesh("cube", vertices, indices);

// 4. Shader'ları yükle ve pipeline oluştur
SDL_GPUShader* vertShader = loadShader("basic_3d.vert.dxil");
SDL_GPUShader* fragShader = loadShader("basic_3d.frag.dxil");
AssetManager::getInstance().createPipeline("default", vertShader, fragShader, swapchainFormat);

// 5. Entity oluştur
auto entity = entityManager.addEntity("cube");
entity->cMesh = CMesh("cube", "default");
entity->cTransform = CTransform(glm::vec3(0, 0, -5));

// 6. Render loop
while (running) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUTexture* swapchain = nullptr;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain, nullptr, nullptr);

    RenderSystem::beginRenderPass(cmd, swapchain, depthTexture);
    RenderSystem::setViewport(0, 0, 1280, 720);
    RenderSystem::update(entityManager, cmd, viewMatrix, projectionMatrix);
    RenderSystem::endRenderPass();

    SDL_SubmitGPUCommandBuffer(cmd);
}
```

---

## 📚 Referanslar

- [SDL_GPU Official Wiki](https://wiki.libsdl.org/SDL3/CategoryGPU)
- [Moonside Games - SDL_GPU Concepts: Cycling](https://moonside.games/posts/sdl-gpu-concepts-cycling/)
- [Moonside Games - SDL_GPU Sprite Batcher](https://moonside.games/posts/sdl-gpu-sprite-batcher/)
- [SDL_GPU Examples](https://github.com/libsdl-org/SDL/tree/main/examples/gpu)

---

## ⚠️ Önemli Notlar

1. **Shader Formatları**: SDL_GPU her platform için farklı shader formatı gerektirir:
   - Windows: DXIL (DirectX 12)
   - macOS/iOS: Metal
   - Linux/Android: SPIRV (Vulkan)

2. **Coordinate System**: SDL_GPU left-handed coordinate system kullanır:
   - NDC: (-1,-1) sol alt, (1,1) sağ üst, Z: [0,1]
   - Viewport: (0,0) sol üst, +Y aşağı
   - Texture: (0,0) sol üst, +Y aşağı

3. **Debugging**: RenderDoc (Windows/Linux) veya Xcode Metal Debugger (macOS) kullanın

4. **Validation Layers**: 
   - D3D12: Graphics Tools (Windows Optional Features)
   - Vulkan: Vulkan SDK
   - Metal: Xcode'dan çalıştır

---

**Astral Engine - Modern 3D Rendering with SDL_GPU 3.4.4**
