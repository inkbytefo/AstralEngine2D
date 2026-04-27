# ASTRAL ENGINE - Kod Tabanı Analiz Raporu

**Tarih:** 27 Nisan 2026  
**Analiz Eden:** Claude (Haiku 4.5)  
**Proje:** Astral Engine 2D/3D  
**Dil:** C++20

---

## 1. GENEL BAKIŞ

Astral Engine, SDL3 ve modern C++ (C++20) kullanılarak geliştirilmiş, hafif ve modüler bir oyun motoru iskeletidir. Proje, **Veri Odaklı Tasarım (Data-Oriented Design)** ve **Entity-Component-System (ECS)** mimarisini temel alır. Kod tabanı, 2D sprite tabanlı oyunlardan 3D mesh rendering'e kadar çeşitli oyun türlerini destekleyecek şekilde tasarlanmıştır.

**Teknoloji Stack:**
- **Grafik API:** SDL3 GPU (Vulkan backend)
- **Matematik:** GLM (OpenGL Mathematics)
- **UI:** ImGui + ImGuizmo
- **Asset Format:** glTF 2.0 (cgltf kütüphanesi ile)
- **Shader Format:** GLSL → SPIR-V (glslc derleyicisi)
- **Build Sistemi:** CMake 3.10+

**Olgunluk Seviyesi:** Erken aşama (Alpha). Temel ECS altyapısı ve render pipeline'ı tamamlanmış, ancak birçok sistem kısmi implementasyon durumundadır.

---

## 2. DİZİN YAPISI

```
Astral/
├── engine/
│   ├── core/                    # Motorun çekirdeği
│   │   ├── app.h/cpp            # Ana uygulama sınıfı, SDL yönetimi
│   │   ├── entity_manager.h/cpp # Entity yaşam döngüsü yönetimi
│   │   ├── system_manager.h/cpp # Sistem kayıt ve çalıştırma
│   │   ├── scene.h              # Sahne temel sınıfı (soyut)
│   │   ├── asset_manager.h      # GPU kaynakları (mesh, texture, pipeline)
│   │   ├── gltf_loader.h/cpp    # glTF 2.0 model yükleme
│   │   ├── scene_serializer.h   # Sahne kaydetme/yükleme
│   │   ├── shader_loader.h      # SPIR-V shader yükleme
│   │   ├── sound_manager.h      # Ses yönetimi (stub)
│   │   ├── level_loader.h       # Level JSON yükleme (stub)
│   │   └── json.hpp             # nlohmann/json kütüphanesi
│   │
│   ├── ecs/                     # Entity-Component-System
│   │   ├── entity.h             # Entity sınıfı (ID, tag, component tuple)
│   │   ├── components.h         # Tüm component tanımları
│   │   └── trait.h              # Trait (script-like behavior) arayüzü
│   │
│   ├── systems/                 # Tüm sistem implementasyonları
│   │   ├── system.h             # ISystem arayüzü
│   │   ├── render_system.h/cpp  # 3D rendering (MVP, PBR)
│   │   ├── physics_system.h     # Basit kinematik (velocity → position)
│   │   ├── input_system.h       # Keyboard/Mouse input
│   │   ├── camera_system.h      # Free-look kamera (WASD + sağ tık)
│   │   ├── transform_system.h   # Scene graph (parent-child hiyerarşi)
│   │   ├── trait_system.h/cpp   # Trait çalıştırma
│   │   ├── lifespan_system.h    # Entity ömrü (fade-out)
│   │   └── text_system.h        # Text rendering (stub)
│   │
│   ├── renderer/                # Rendering abstraction
│   │   ├── graphics_device.h    # IGraphicsDevice arayüzü
│   │   ├── renderer.h           # IRenderer arayüzü
│   │   ├── sdl3_graphics_device.h/cpp  # SDL3 GPU implementasyonu
│   │   └── sdl3_renderer.h/cpp         # SDL3 render pass yönetimi
│   │
│   ├── editor/                  # Editor UI (ImGui)
│   │   ├── editor_manager.h/cpp # Editor ana yöneticisi
│   │   └── panels/              # ImGui panelleri
│   │       ├── viewport_panel.h/cpp        # 3D viewport
│   │       ├── scene_hierarchy_panel.h/cpp # Entity ağacı
│   │       ├── properties_panel.h/cpp      # Component inspector
│   │       ├── console_panel.h/cpp         # Debug konsolu
│   │       └── content_browser_panel.h/cpp # Asset browser
│   │
│   ├── math/
│   │   └── vertex.h             # GPU vertex yapısı (64 bytes)
│   │
│   └── vendor/                  # Harici kütüphaneler
│       ├── imgui/               # ImGui + backends
│       ├── glm/                 # GLM matematik
│       └── cgltf/               # glTF parser
│
├── games/
│   └── sandbox/                 # Test oyunu
│       ├── main.cpp
│       ├── sandbox_scene.h/cpp
│       └── pong_level.json
│
├── assets/
│   ├── shaders/                 # GLSL → SPIR-V
│   │   ├── basic_3d.vert/frag.glsl
│   │   ├── pbr.vert/frag.glsl
│   │   └── *.spv (derlenmiş)
│   ├── textures/                # PNG görseller
│   ├── models/                  # glTF modeller
│   ├── sounds/                  # WAV ses dosyaları
│   ├── fonts/                   # TTF fontlar
│   └── levels/                  # JSON level dosyaları
│
├── docs/                        # Teknik dokümantasyon
│   ├── architecture.md
│   ├── graphics_pipeline.md
│   ├── editor_design.md
│   └── getting_started.md
│
├── CMakeLists.txt               # Build konfigürasyonu
├── CMakePresets.json
└── README.md
```

---

## 3. ECS MİMARİSİ

### 3.1 Genel Yapı

Astral Engine, **Tuple-based ECS** mimarisi kullanır. Bu, tüm component'ların bir `std::tuple` içinde saklanmasını ve compile-time type safety sağlamasını anlamına gelir.

```cpp
// Entity sınıfı içinde:
using ComponentTuple = std::tuple<
    CTransform, CShape, CBBox, CInput, CLifeSpan, CText, 
    CSprite, CMesh, CCamera, CLight, CFreeLook, CTrait
>;

class Entity {
    uint32_t m_id;
    std::string m_tag;
    bool m_active;
    ComponentTuple m_components;  // Tüm component'lar burada
};
```

**Avantajlar:**
- Compile-time type checking
- Cache-friendly (tüm component'lar bir entity'de)
- Basit ve anlaşılır

**Dezavantajlar:**
- Sparse set değil (boş component'lar da bellek tutar)
- Archetype-based query yok (view() her seferinde filtreler)
- Scalability sınırlı (component sayısı arttıkça tuple büyür)

### 3.2 Entity Yönetimi

**EntityManager** sorumluluğu:
- Entity oluşturma/silme (gecikmeli işlem)
- ID üretimi (uint32_t counter)
- Tag-based filtreleme
- Component-based view (query)

```cpp
class EntityManager {
    EntityVec m_entities;        // Aktif entity'ler
    EntityVec m_toAdd;           // Sonraki frame'de eklenecekler
    EntityMap m_entityMap;       // Tag → EntityVec
    uint32_t m_totalEntites;     // ID counter
    
    // Gecikmeli işlem (Delayed Dispatch)
    void update() {
        // m_toAdd'deki entity'leri m_entities'e taşı
        // Silinmiş entity'leri temizle
    }
};
```

**Gecikmeli İşlem Avantajı:** Oyun döngüsü sırasında iterator invalidation'dan kaçınır.

### 3.3 Component Sistemi

**12 Component Tanımı:**

| Component | Alanlar | Amaç |
|-----------|---------|------|
| **CTransform** | pos, velocity, scale, rotation, parent, children, globalMatrix | 3D pozisyon, hız, ölçek, rotasyon, scene graph |
| **CShape** | width, height, r, g, b, a | 2D şekil ve renk (debug/UI) |
| **CBBox** | width, height | Bounding box (collision) |
| **CInput** | up, down, left, right | Keyboard input state |
| **CSprite** | texture, srcRect, angle | 2D sprite rendering |
| **CMesh** | meshName, materialName | 3D mesh referansı |
| **CCamera** | view, projection, aspectRatio, isActive | Kamera matrisleri |
| **CFreeLook** | yaw, pitch, speed, sensitivity, mouseState | Free-look kamera kontrol |
| **CLight** | type, color, intensity, direction, range, cutoff | PBR ışık (Directional/Point/Spot) |
| **CLifeSpan** | remaining, total | Entity ömrü (fade-out) |
| **CText** | text, font, color, texture, needsUpdate | Text rendering |
| **CTrait** | traits (vector) | Script-like behavior'lar |

### 3.4 System Mimarisi

**ISystem Arayüzü:**
```cpp
class ISystem {
    virtual void init(EntityManager&) {}
    virtual void update(EntityManager&, float deltaTime) = 0;
    virtual void onEvent(const SDL_Event&) {}
    virtual void shutdown() {}
    virtual int32_t getPriority() const { return 0; }
    virtual const char* getName() const = 0;
};
```

**SystemManager:**
- Sistem kayıt ve çalıştırma
- Priority-based sıralama
- Event dispatch

### 3.5 Query Pattern (View)

```cpp
// Component-based filtering
auto entities = entityManager.view<CTransform, CCamera>();

// Variadic template ile compile-time check
template <typename... T>
EntityVec view() {
    EntityVec result;
    for (auto& entity : m_entities) {
        if (entity->isActive() && (entity->has<T>() && ...)) {
            result.push_back(entity);
        }
    }
    return result;
}
```

**Sorun:** Her çağrıda O(n) filtreleme yapılır. Archetype caching yok.

---

## 4. MEVCUT SİSTEMLER

| Sistem | Dosya | Durum | Bağımlı Component'lar | Açıklama |
|--------|-------|-------|----------------------|----------|
| **RenderSystem** | render_system.h/cpp | ✅ Kısmi | CMesh, CTransform, CCamera, CLight | 3D mesh rendering, MVP matrisleri, PBR uniform'ları |
| **PhysicsSystem** | physics_system.h | ✅ Tamamlanmış | CTransform | Basit kinematik (pos += velocity * dt) |
| **InputSystem** | input_system.h | ✅ Tamamlanmış | CInput, CFreeLook | Keyboard/Mouse input, action mapping |
| **CameraSystem** | camera_system.h | ✅ Tamamlanmış | CCamera, CFreeLook, CTransform, CInput | Free-look kamera (WASD + sağ tık) |
| **TransformSystem** | transform_system.h | ✅ Tamamlanmış | CTransform | Scene graph (parent-child hiyerarşi, globalMatrix) |
| **LifespanSystem** | lifespan_system.h | ✅ Tamamlanmış | CLifeSpan, CShape | Entity ömrü, fade-out efekti |
| **TextSystem** | text_system.h | ⚠️ Stub | CText, CTransform | Text rendering (boş implementasyon) |
| **TraitSystem** | trait_system.h/cpp | ⚠️ Kısmi | CTrait | Trait çalıştırma (onInit, onUpdate, onCollision) |

**Çalışma Sırası (Priority):**
1. InputSystem (-100) → Keyboard/Mouse input
2. PhysicsSystem (10) → Kinematik (tek otorite)
3. TransformSystem (0) → Scene graph
4. LifespanSystem (30) → Ömür azaltma
5. CameraSystem (20) → Kamera güncelleme
6. TextSystem (90) → Text caching
7. RenderSystem (100) → Rendering

---

## 5. MEVCUT COMPONENT'LAR

| Component | Alanlar | Kullanan Sistemler | Varsayılan Değer |
|-----------|---------|-------------------|-----------------|
| **CTransform** | pos(0,0,0), velocity(0,0,0), scale(1,1,1), rotation(0,0,0), parent, children, globalMatrix | Tüm sistemler | Identity matrix |
| **CShape** | width(0), height(0), r/g/b/a(255) | RenderSystem (2D) | Beyaz, 0x0 |
| **CBBox** | width(0), height(0) | Collision (stub) | 0x0 |
| **CInput** | up/down/left/right(false) | InputSystem, CameraSystem | Tüm false |
| **CSprite** | texture(nullptr), srcRect(0,0,0,0), angle(0) | RenderSystem (2D) | Boş |
| **CMesh** | meshName(""), materialName("default") | RenderSystem (3D) | Default material |
| **CCamera** | view/projection(identity), aspectRatio(16:9), isActive(true) | RenderSystem, CameraSystem | Identity matrices |
| **CFreeLook** | yaw(-90), pitch(0), speed(10), sensitivity(0.1), mouseState | CameraSystem, InputSystem | Standart FPS kamera |
| **CLight** | type(Directional), color(1,1,1), intensity(1), direction(0,-1,0), range(20) | RenderSystem | Directional ışık |
| **CLifeSpan** | remaining(0), total(0) | LifespanSystem | Sonsuz ömür |
| **CText** | text(""), font(nullptr), color(255,255,255,255), texture(nullptr), needsUpdate(true) | TextSystem, RenderSystem | Boş metin |
| **CTrait** | traits (vector) | TraitSystem | Boş vector |

---

## 6. ÇALIŞAN ÖZELLİKLER

### Rendering
- ✅ 3D mesh rendering (glTF modeller)
- ✅ PBR material system (Albedo, Normal, Metallic-Roughness)
- ✅ Multiple light types (Directional, Point, Spot)
- ✅ MVP matrix transformations
- ✅ Shader compilation (GLSL → SPIR-V)
- ✅ Mega-buffer architecture (50MB vertex, 20MB index)
- ✅ Texture loading (PNG via SDL_image)
- ⚠️ 2D sprite rendering (CSprite component var ama render kodu eksik)

### Input Handling
- ✅ Keyboard input (WASD, custom action mapping)
- ✅ Mouse input (relative motion, button detection)
- ✅ Free-look camera (right-click + mouse)
- ✅ Action callback system

### Scene Management
- ✅ Entity-based scene graph
- ✅ Parent-child transform hierarchy
- ✅ Scene serialization (JSON)
- ✅ glTF model loading with hierarchy

### Asset Management
- ✅ GPU mesh upload (mega-buffer)
- ✅ Texture loading and caching
- ✅ Shader loading (SPIR-V)
- ✅ Pipeline creation and caching
- ✅ Material management
- ✅ Font loading (TTF)
- ✅ Fallback textures (white, black, default normal)

### Physics
- ✅ Basit kinematik (velocity-based movement)
- ❌ Collision detection (stub)
- ❌ Rigid body dynamics
- ❌ Gravity

### Audio
- ❌ Sound playback (SDL_mixer bağlı ama implementasyon yok)
- ❌ Music management

### Editor
- ✅ ImGui integration
- ✅ Viewport panel (3D view)
- ✅ Scene hierarchy panel
- ✅ Properties panel (component inspector)
- ✅ Console panel
- ✅ Content browser panel

---

## 7. EKSİKLER VE SONRAKİ ADIMLAR

### Yarım Kalmış Implementasyonlar

1. **TextSystem** - Boş implementasyon
   - CText component'ı var ama render kodu yok
   - Font caching mekanizması eksik
   - Metin texture'ı güncelleme mantığı eksik

2. **Collision System** - Tamamen eksik
   - CBBox component var ama kullanılmıyor
   - checkCollision() helper fonksiyonu var ama sistem yok
   - Collision callbacks (onCollision) tanımlanmış ama çalışmıyor

3. **Sound Manager** - Stub
   - SDL_mixer bağlı ama implementasyon yok
   - Sound playback, music management yok

4. **Level Loader** - Stub
   - LevelLoader sınıfı tanımlanmış ama boş
   - JSON level parsing yok

5. **2D Sprite Rendering** - Kısmi
   - CSprite component var
   - RenderSystem'de 3D mesh rendering var ama 2D sprite kodu eksik

6. **Trait System** - Kısmi
   - ITrait arayüzü tanımlanmış
   - TraitSystem::update() implementasyonu eksik
   - Collision callback'leri çalışmıyor

### TODO/FIXME Yorum Satırları

```cpp
// CSprite'da:
// SDL_GPUTexture boyutu farkli alinir
// (Kod commented out, boyut alma mekanizması eksik)

// CText'te:
// SDL_ReleaseGPUTexture(m_gpuDevice, texture); 
// (Device referansi lazım - cleanup eksik)
```

### Stub Fonksiyonlar

- `LevelLoader` - Tüm metodlar boş
- `SoundManager` - Tüm metodlar boş
- `TextSystem::update()` - Boş loop
- `TraitSystem::update()` - Implementasyon eksik

### Mimari Tutarsızlıklar

1. **RenderSystem::render()** - Incomplete (render_system.cpp 100 satırda kesilmiş)
2. **Scene::render()** - Soyut ama implementasyon yok
3. **EditorManager** - Tanımlanmış ama detaylar bilinmiyor

---

## 8. TEKNİK BORÇ

### Memory Management

**Sorunlar:**
- Raw pointer'lar kullanılıyor (SDL_GPUTexture*, SDL_GPUBuffer*, vb.)
- Manual cleanup gerekli (AssetManager::cleanup())
- Potential memory leak'ler:
  - CText::texture cleanup eksik
  - Entity silinirken component cleanup yok
  - Shader'lar manuel release gerekli

**Öneriler:**
- RAII wrapper'ları oluştur (SDL kaynakları için)
- Smart pointer'lar kullan (unique_ptr, shared_ptr)
- Destructor'larda otomatik cleanup

### Hata Yönetimi

**Sorunlar:**
- Hata handling minimal (SDL_Log() ile logging)
- Exception yok
- Null pointer check'leri eksik bazı yerlerde
- GPU device fail durumunda cascade failure

**Öneriler:**
- Result type (Result<T, Error>) kullan
- Exception safety guarantee'leri tanımla
- Graceful degradation (fallback texture'lar gibi)

### Naming Convention

**Tutarlılık:** Genel olarak iyi
- Component'lar: `C` prefix (CTransform, CCamera)
- System'ler: `System` suffix (RenderSystem, InputSystem)
- Private member'lar: `m_` prefix (m_entities, m_gpuDevice)

**Tutarsızlıklar:**
- `ISystem`, `IRenderer`, `IGraphicsDevice` - Interface prefix tutarlı
- `EntityVec`, `EntityMap` - Type alias'lar global namespace'de

### Tekrarlayan Kod (DRY İhlalleri)

1. **RenderSystem'de state tracking** - Pipeline/Material binding tekrarlanıyor
2. **Component has() check'leri** - Birçok yerde tekrarlanan pattern

### Performans Riski Yaratan Alanlar

1. **EntityManager::view()** - O(n) filtreleme her çağrıda
   - Çözüm: Archetype caching veya sparse set

2. **RenderSystem::prepare()** - Sorting her frame
   - Çözüm: Incremental sorting veya batch grouping

3. **Transform hierarchy update** - Recursive, deep hierarchy'de yavaş
   - Çözüm: Iterative update veya job system

4. **Texture upload** - Synchronous, blocking
   - Çözüm: Async upload queue

5. **Component tuple** - Sparse set değil, boş component'lar bellek tutar
   - Çözüm: Archetype-based storage

---

## 9. MİMARİ ÖNERİLER

### 1. Archetype-Based ECS (Uzun Vadeli)

**Mevcut:** Tuple-based, sparse component storage  
**Önerilen:** Archetype-based (SoA - Structure of Arrays)

```cpp
// Örnek: Archetype pattern
struct Archetype {
    std::vector<CTransform> transforms;
    std::vector<CMesh> meshes;
    std::vector<CCamera> cameras;
    // ... diğer component'lar
};

// Avantajlar:
// - Cache-friendly (SoA layout)
// - Hızlı query'ler
// - Scalable
```

### 2. Job System (Parallelization)

**Mevcut:** Sequential system update  
**Önerilen:** Job-based parallelization

```cpp
// Örnek:
SystemManager::update() {
    // Job queue'ya sistemleri ekle
    jobQueue.enqueue(physicsSystem);
    jobQueue.enqueue(transformSystem);
    jobQueue.enqueue(renderSystem);
    
    // Parallel execute
    jobQueue.execute();
}
```

### 3. Event System (Decoupling)

**Mevcut:** Direct system communication  
**Önerilen:** Event-based messaging

```cpp
// Örnek:
class EventBus {
    void publish(const Event& e);
    void subscribe(EventType, Callback);
};

// Collision event örneği
struct CollisionEvent {
    Entity* a;
    Entity* b;
};
```

### 4. Resource Handle System

**Mevcut:** Raw pointer'lar  
**Önerilen:** Handle-based resource management

```cpp
// Örnek:
struct MeshHandle {
    uint32_t id;
    uint32_t generation; // Versioning
};

// Avantajlar:
// - Dangling pointer'lar yok
// - Resource reuse tracking
```

### 5. Serialization Framework

**Mevcut:** Scene serializer (kısmi)  
**Önerilen:** Reflection-based serialization

```cpp
// Örnek:
REFLECT_COMPONENT(CTransform) {
    REFLECT_FIELD(pos);
    REFLECT_FIELD(velocity);
    REFLECT_FIELD(scale);
};

// Otomatik JSON serialization
```

---

## 10. ÖZET TABLO

| Kategori | Durum | Notlar |
|----------|-------|--------|
| **ECS Core** | ✅ Tamamlanmış | Tuple-based, gecikmeli işlem, view pattern |
| **Rendering** | ✅ Kısmi | 3D mesh, PBR, shader compilation var; 2D sprite eksik |
| **Input** | ✅ Tamamlanmış | Keyboard, mouse, free-look kamera |
| **Physics** | ⚠️ Minimal | Basit kinematik; collision, gravity yok |
| **Audio** | ❌ Stub | SDL_mixer bağlı ama implementasyon yok |
| **Scene Management** | ✅ Tamamlanmış | Entity hierarchy, serialization |
| **Asset Pipeline** | ✅ Tamamlanmış | Mesh, texture, shader, font loading |
| **Editor** | ✅ Kısmi | ImGui panels var; full functionality eksik |
| **Collision** | ❌ Eksik | Component var ama sistem yok |
| **Traits/Scripting** | ⚠️ Kısmi | Interface tanımlanmış; execution eksik |

---

## 11. HEMEN YAPILABILIR İYİLEŞTİRMELER

### 1. TextSystem Implementasyonu Tamamlama
**Neden:** CText component var ama render kodu yok  
**Nasıl:** RenderSystem'e text rendering pass ekle, CText texture caching'i implement et  
**Etki:** UI text rendering çalışır hale gelir

### 2. Collision System Oluşturma
**Neden:** CBBox component var ama kullanılmıyor, checkCollision() helper var  
**Nasıl:** CollisionSystem oluştur, AABB collision detection implement et, onCollision callback'lerini çalıştır  
**Etki:** Temel collision detection ve response sistemi çalışır

---

**Rapor Sonu**
