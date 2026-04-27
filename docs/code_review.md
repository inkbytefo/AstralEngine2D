# Astral Engine: Kod Kalitesi ve Mimari İnceleme Raporu

Mevcut kaynak kodlarımızı C++ pratikleri, temiz kod prensipleri ve oyun motoru mimarisi açısından detaylıca inceledim. Motorun şu anki durumu "Hızlı Prototipleme" aşamasından "Kurumsal Mimari" aşamasına geçiş eşiğindedir.

---

## 1. Tespit Edilen "Kötü Kokular" (Code Smells)

### 1.1. God Class (Tanrı Nesne) Riski: `AssetManager`
`AssetManager` şu an motorun en ağır sıklet sınıfı. 
*   **Sorun:** Meshes, Textures, Shaders, Pipelines, Materials, Fonts ve Mega-Buffer yönetimi tek bir sınıfta.
*   **SRP İhlali:** Bir sınıfın değişmesi için birden fazla sebep olmamalıdır. Şu an Pipeline yapısını değiştirseniz de, Doku yükleme mantığını değiştirseniz de bu sınıfa dokunmanız gerekiyor.
*   **Öneri:** `AssetManager`'ı bir "Registry" (Kayıt Defteri) haline getirip, altına `TextureLoader`, `MeshRegistry`, `ShaderCompiler` gibi uzmanlaşmış alt sınıflar koymalıyız.

### 1.2. Sıkı Bağlılık (Tight Coupling): Singleton Kullanımı
`AssetManager::getInstance()` motorun her yerine (RenderSystem, GLTFLoader, SandboxScene) sızmış durumda.
*   **Sorun:** Global state (küresel durum) kullanımı test edilebilirliği zorlaştırır ve kodun takibini güçleştirir.
*   **Öneri:** "Dependency Injection" (Bağımlılık Enjeksiyonu) kullanarak, sistemlere ihtiyaç duydukları managerları constructor veya metod parametresi üzerinden geçmeliyiz.

### 1.3. RAII ve Akıllı İşaretçi Eksikliği
SDL kaynakları (Texture, Buffer, Font) hala manuel `Release` veya `cleanup` metodları ile yönetiliyor.
*   **Sorun:** Manuel bellek yönetimi her zaman sızıntı (leak) riskidir.
*   **Öneri:** SDL kaynakları için `std::unique_ptr` ve `std::shared_ptr` ile özel silici (custom deleter) kullanılmalıdır. Örneğin:
    ```cpp
    using GPUTexturePtr = std::unique_ptr<SDL_GPUTexture, void(*)(SDL_GPUTexture*)>;
    ```

---

## 2. Mimari İhlaller

### 2.1. Header-Heavy Logic: `GLTFLoader.h`
*   **Durum:** Karmaşık GLTF ayrıştırma mantığı büyük oranda header dosyasında.
*   **Sorun:** Proje büyüdükçe derleme süreleri (compile times) logaritmik olarak artacaktır.
*   **Öneri:** Tüm implementasyon detayları `.cpp` dosyasına taşınmalı, header sadece arayüzü (interface) sunmalıdır.

### 2.2. RenderSystem ve AssetManager İlişkisi
*   **Durum:** `RenderSystem` doğrudan `AssetManager`'dan doku çekiyor.
*   **Sorun:** Render aşamasında asset araması (`std::map::find`) yapmak CPU tarafında darboğaz yaratabilir.
*   **Öneri:** "Render Packet" veya "Draw Command" yapısına geçilerek, render edilmeden önce tüm kaynaklar (handle seviyesinde) hazırlanmalı, RenderSystem sadece ham veriyi basmalıdır.

---

## 3. İyi Uygulamalar (Neler Doğru?)

*   **Mega-Buffer Stratejisi:** Vertex/Index verilerinin tek bir büyük buffer'da toplanması modern GPU API'leri için mükemmel bir pratik.
*   **Entity-Component Ayrımı:** `CTransform`, `CMesh` gibi yapıların sadece veri (POD) içermesi DOD (Data Oriented Design) prensiplerine uygun.
*   **Scene Graph (Hierarchy):** `CTransform` içindeki ebeveyn-çocuk ilişkisi basit ve etkili bir şekilde kurulmuş.

---

## 4. İyileştirme Yol Haritası

1.  **Refactor Phase A:** `AssetManager`'ı parçalara ayır (Texture, Mesh, Material).
2.  **Refactor Phase B:** Manuel `cleanup()` çağrılarını kaldırıp RAII'ye geç.
3.  **Refactor Phase C:** `App` sınıfından Pencere ve GPU Device yönetimini ayır (`Window` ve `GraphicsDevice` sınıfları).
4.  **Refactor Phase D:** `Scene` sınıfını daha modüler hale getirerek sistemleri (System) otomatik kaydeden bir yapı kur.

**Özet:** Motor şu an çalışıyor ve performansı iyi, ancak üzerine "Editor" gibi devasa bir modül eklemeden önce bu mimari borçları (Technical Debt) ödememiz ileride işimizi çok kolaylaştıracaktır.
