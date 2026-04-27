# Grafik ve Render Akış Hattı (Graphics Pipeline)

Astral Engine, modern grafik API'lerinin donanım hızlandırma yeteneklerinden tam anlamıyla faydalanmak için **SDL3_GPU** arka ucunu (Vulkan, DirectX 12, Metal) kullanır. Projeniz şu anda **Vulkan SPIR-V** formatında derlenmiş PBR (Fizik Tabanlı Render) mimarisi kullanmaktadır.

## 1. PBR (Physically Based Rendering)

Oyun dünyasındaki nesnelerin gerçekçi görünmesini sağlamak için "Cook-Torrance BRDF" modeli uygulanmıştır. Bu model, ışığın yüzeyle olan etkileşimini gerçek dünyadaki fizik kurallarına dayanarak hesaplar.

### PBR İş Akışı (Metallic-Roughness)
- **Albedo (Base Color):** Nesnenin saf rengini temsil eder. (Üzerinde gölge veya parlama barındırmaz).
- **Metallic:** Yüzeyin metalik (iletken) mi yoksa yalıtkan (dielektrik) mı olduğunu belirler. Metalse, kendi rengini yansıtır; değilse sadece dışarıdan gelen ışığı yansıtır.
- **Roughness:** Yüzeyin pürüzlülüğünü belirler. Pürüzsüz yüzeyler ayna gibi yansıma yaparken, pürüzlü yüzeyler ışığı dağıtır (Diffuse).
- **Normal Map:** Düz yüzeylere girinti ve çıkıntı hissi vermek için ışığın kırılma açısını manipüle eder.

## 2. Vertex ve Shader Mimarisi

Performansı maksimize etmek için `Vertex` yapısı 64-byte olarak optimize edilmiştir (GPU Cache Line uyumluluğu).

```cpp
struct Vertex {
    glm::vec3 pos;      // Location 0 (Pozisyon)
    glm::vec3 normal;   // Location 1 (Yüzey Yönü)
    glm::vec2 uv;       // Location 2 (Doku Koordinatı)
    glm::vec4 tangent;  // Location 3 (Normal Map Teğeti ve Yönelim)
    glm::vec4 color;    // Location 4 (Renk)
};
```

### Shader'lar
Shader'lar GLSL (OpenGL Shading Language) ile yazılır ve `CMake` tarafında otomatik olarak `glslc` aracı ile makine koduna (SPIR-V) dönüştürülür.
1. `pbr.vert.glsl`: Modelin dünyadaki pozisyonunu hesaplar. Işık hesaplamaları için Normal, Tangent ve Bitangent vektörlerini üretip Fragment shader'a aktarır.
2. `pbr.frag.glsl`: Her bir piksel için Işık Korumunu (Energy Conservation) göz önünde bulundurarak ışık ve renk çarpışmalarını (Fresnel, Geometry, Normal Distribution) hesaplar.

## 3. RenderSystem ve AssetManager
Tüm çizim komutları `RenderSystem::update()` fonksiyonu üzerinden çalışır.
1. **AssetManager**, pipeline (boru hattı), doku (texture) ve materyalleri (material) yükler ve VRAM'de saklar.
2. `RenderSystem` sahnedeki tüm `CMesh` ve `CTransform` bileşenine sahip nesneleri toplar.
3. Aynı pipeline ve materyali kullanan nesneler gruplanarak (State Minimization) ardışık olarak çizilir. Bu yöntem Context Switch (bağlam değiştirme) maliyetini devasa oranda düşürür.
4. Işık ve kamera verileri tek bir seferde (Set 3 Uniform Buffer üzerinden) GPU'ya gönderilir.
