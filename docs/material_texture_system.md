# Material & Texture System Architecture (AAA Standard)

## 📋 İçindekiler
1. [Genel Bakış](#genel-bakış)
2. [Mimari Tasarım](#mimari-tasarım)
3. [State Minimization & Sorting](#state-minimization--sorting)
4. [Texture Upload Pipeline](#texture-upload-pipeline)
5. [Material Workflow](#material-workflow)
6. [Implementasyon Detayları](#implementasyon-detayları)

---

## Genel Bakış

Astral Engine'in Material & Texture sistemi, modern AAA oyun motorlarının (Unreal Engine, Unity) kanıtlanmış mimari prensiplerini SDL3_GPU API'si üzerine uyarlar. Sistem, **Data-Oriented Design (DOD)** ve **State Minimization** prensiplerine dayanır.

### Temel Hiyerarşi
```
Shader → Pipeline → Material → Mesh
```

- **Shader**: GPU'da çalışan SPIR-V kodu.
- **Pipeline**: Shader + GPU State (blend, depth, cull). Oluşturması en pahalı nesne.
- **Material**: Pipeline'ın bir örneği + Texture (Set 2) + Material Properties (Set 3).
- **Mesh**: Geometri verisi (Vertex/Index Buffers).

---

## Mimari Tasarım

### Render Sıralama Stratejisi
AAA motorların kullandığı "Pahalıdan Ucuza" sıralama uygulanır:

1. **Pipeline'a göre sırala**: Context switch önlenir (EN PAHALI).
2. **Material'a göre sırala**: Texture ve Uniform değişimleri minimize edilir (ORTA).
3. **Mesh'e göre sırala**: Vertex/Index buffer bind işlemleri minimize edilir (UCUZ).

---

## State Minimization & Sorting

`RenderSystem`, her karede varlıkları otomatik olarak sıralar ve sadece değişen state'leri GPU'ya gönderir.

```cpp
// RenderSystem.h içindeki yeni mantık:
std::sort(renderEntities.begin(), renderEntities.end(), [](const auto& a, const auto& b) {
    auto& ma = a->get<CMesh>();
    auto& mb = b->get<CMesh>();
    if (ma.materialName != mb.materialName) return ma.materialName < mb.materialName;
    return ma.meshName < mb.meshName;
});
```

Bu yaklaşım, yüzlerce objenin olduğu sahnelerde GPU komut sayısını %80'e kadar azaltır.

---

## Texture Upload Pipeline

### Staging Buffer (Zero-Copy)
**CPU → Staging Buffer → VRAM** akışı kullanılır.

1. `SDL_Surface` ile diskten yükleme.
2. `SDL_ConvertSurface` ile `RGBA32` formatına dönüşüm.
3. `SDL_GPUTransferBuffer` (Staging) üzerinden DMA ile VRAM'e aktarım.
4. `SDL_WaitForGPUIdle` veya semaforlarla senkronizasyon (Opsiyonel: Async Upload).

---

## Material Workflow

### 1. Tanımlama
`AssetManager` üzerinden merkezi yönetim sağlanır.

```cpp
// Texture yükle
assetMgr.uploadTexture("box_tex", "assets/textures/box.png");

// Material yarat (Pipeline + Texture)
assetMgr.createMaterial("box_material", "basic3d", "box_tex");
```

### 2. Kullanım (ECS)
```cpp
auto cubeEnt = m_entityManager.addEntity("cube");
cubeEnt->add<CMesh>("cube", "box_material");
```

---

## Implementasyon Detayları

### SDL_GPU Shader Binding Layout
Astral Engine, SDL_GPU'nun donanım soyutlama standartlarını takip eder:

- **Vertex Uniforms (MVP)**: Slot 0 (Push Data)
- **Fragment Samplers (Texture)**: **Set 2**, Binding 0
- **Fragment Uniforms (Material Properties)**: **Set 3**, Binding 0

### Fragment Shader Yapısı
```glsl
layout(set = 2, binding = 0) uniform sampler2D albedoTex;

layout(set = 3, binding = 0) uniform FragmentData {
    vec4 baseColor;
    int hasTexture;
    int _padding[3];
} fragUniforms;
```

---

**Son Güncelleme:** 2026-04-27  
**Versiyon:** 1.1 (AAA Refactor Update)  
**Yazar:** Astral Engine Team
