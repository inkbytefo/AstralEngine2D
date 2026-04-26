# SDL3 GPU API (v3.4.4) Teknik Analiz ve Rehber

Bu döküman, Astral Engine'in 2D'den 3D'ye geçiş sürecinde kullanılan SDL3 GPU API'sinin temel çalışma prensiplerini ve mimarisini özetler.

## 1. Mimari Genel Bakış
SDL3 GPU, modern grafik API'leri (Vulkan, Metal, D3D12) üzerine inşa edilmiş, explicit (açık) bir grafik API'sidir. Eski `SDL_Renderer`'ın aksine, kaynak yönetimi ve komut gönderimi üzerinde tam kontrol sağlar.

### Temel Nesneler
- **`SDL_GPUDevice`**: Grafik donanımına erişim sağlayan ana nesne.
- **`SDL_GPUCommandBuffer`**: Komutların toplandığı ve GPU'ya gönderildiği yapı.
- **`SDL_GPUGraphicsPipeline`**: Shaderlar, harmanlama (blending), vertex formatı ve rasterizer ayarlarını içeren, oluşturulduktan sonra değiştirilemeyen (immutable) nesne.

## 2. Render Döngüsü (Frame Lifecycle)
Bir kareyi çizmek için izlenen standart yol:

1. **Komut Tamponu Alımı**: `SDL_AcquireGPUCommandBuffer(device)`
2. **Swapchain Texture Bekleme/Alım**: `SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &texture, &w, &h)`
3. **Render Pass Başlatma**: `SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, depthStencilInfo)`
4. **Pipeline ve Kaynak Bağlama**: `SDL_BindGPUGraphicsPipeline(renderPass, pipeline)`
5. **Çizim Komutu**: `SDL_DrawGPUPrimitives(renderPass, vertexCount, instanceCount, firstVertex, firstInstance)`
6. **Render Pass Bitirme**: `SDL_EndGPURenderPass(renderPass)`
7. **Gönderim ve Sunum**: `SDL_SubmitGPUCommandBuffer(commandBuffer)` (Swapchain texture alındıysa otomatik sunum yapılır)

## 3. Kaynak Yönetimi
### Veri Yükleme (Data Upload) Deseni
GPU belleğine veri yazmak için "Staging" mantığı kullanılır:
1. `SDL_GPUTransferBuffer` (CPU-GPU köprüsü) oluşturulur.
2. Veri `SDL_MapGPUTransferBuffer` ile köprüye kopyalanır.
3. `SDL_BeginGPUCopyPass` ile veri köprüden asıl `SDL_GPUBuffer`'a aktarılır.

### Shader Formatları
SDL3 GPU şu formatları destekler:
- **SPIR-V**: Vulkan için.
- **MSL**: Metal (macOS/iOS) için.
- **DXIL**: Direct3D 12 için.

## 4. Önemli Fonksiyonlar ve Yapılar
- `SDL_CreateGPUDevice`: Cihazı başlatır.
- `SDL_ClaimWindowForGPUDevice`: Pencereyi GPU kullanımına açar.
- `SDL_GPUColorTargetInfo`: Ekran temizleme rengi ve yükleme/saklama operasyonlarını (load/store ops) tanımlar.
- `SDL_GPUGraphicsPipelineCreateInfo`: Tüm grafik hattını tek seferde tanımlayan devasa yapı.

## 5. Astral Engine İpuçları
- **Culling**: Varsayılan olarak arka yüzeyler çizilmeyebilir. `SDL_GPU_CULLMODE_NONE` kullanarak hata ayıklama yapılabilir.
- **Winding Order**: Üçgenlerin noktalarının sırası (saat yönü veya tersi) görünürlüğü etkiler.
- **Format**: Swapchain formatı `SDL_GetGPUSwapchainTextureFormat` ile sorgulanmalıdır.
