# Astral Engine: SDL_GPU 3D Rendering Mimarisi

Bu doküman, SDL 3.4.4'ün modern `SDL_GPU` API'sini kullanarak Astral Engine için yüksek performanslı bir 3D Rendering iskeleti oluşturma stratejisini açıklar.

## 1. Mimari Kararlar

- **Staging (Transfer) Buffers**: Veriyi doğrudan VRAM'e (Static Buffer) yazmak yerine, `SDL_GPUTransferBuffer` kullanarak "Copy Pass" üzerinden aktarıyoruz. Bu, GPU'nun veriyi en hızlı şekilde (Local VRAM) işlemesini sağlar ve CPU-GPU senkronizasyonunu optimize eder.
- **Push Uniforms**: MVP matrisleri gibi her obje için değişen küçük veriler için `SDL_PushGPUVertexUniformData` kullanıyoruz. Bu, Vulkan'daki Push Constants mantığına yakındır ve ayrı Uniform Buffer yönetimi yükünü ortadan kaldırır.
- **Pipeline Caching**: Pipeline oluşturma maliyeti yüksektir. `AssetManager` içinde shader, blend mode ve depth state kombinasyonlarını cache'leyerek çalışma anında sadece bind işlemi yapıyoruz.
- **State Sorting**: `RenderSystem` içinde objeleri Pipeline bazlı sıralayarak gereksiz bind işlemlerini (context switching) minimize ediyoruz.

## 2. Uygulama Detayları

### A. Vertex Yapısı (`engine/math/vertex.h`)
GPU dostu interleaved layout.

```cpp
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv;
};
```

### B. Kaynak Yükleme (Resource Upload)
`AssetManager::uploadMesh` fonksiyonu şu adımları izler:
1. `SDL_CreateGPUTransferBuffer` ile geçici bellek alanı açılır.
2. `SDL_MapGPUTransferBuffer` ile CPU verisi buraya kopyalanır.
3. `SDL_CreateGPUBuffer` ile kalıcı Vertex ve Index buffer'lar oluşturulur.
4. `SDL_BeginGPUCopyPass` ile staging alanından kalıcı buffer'lara transfer emri verilir.
5. Command Buffer submit edildikten sonra staging buffer release edilir.

### C. Render Döngüsü (State Management)
`RenderSystem::update` içerisinde state change optimizasyonu:
```cpp
if (mesh.pipeline != lastPipeline) {
    SDL_BindGPUGraphicsPipeline(pass, mesh.pipeline);
    lastPipeline = mesh.pipeline;
}
```
Bu kontrol, binlerce draw call içeren sahnelerde CPU overhead'ini ciddi oranda düşürür.

## 3. SDL_GPU Avantajları
- **Explicit control**: Bellek ve komut kuyruğu üzerinde tam kontrol.
- **Unified Shaders**: SDL_GPU, shader'ları farklı backend'ler (Vulkan, D3D12, Metal) için otomatik yönetebilir veya SPIR-V/DXIL gibi formatları destekler.
- **Modern Syntax**: Geleneksel SDL_Renderer'a göre çok daha düşük seviyeli ama modern GPU mimarilerine tam uyumlu.
