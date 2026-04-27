# Astral Engine: Editor Mimari Tasarımı (27.04.2026)

Bu doküman, Astral Engine için planlanan görsel editörün (Astral Editor) teknik mimarisini, arayüz düzenini ve uygulama stratejisini detaylandırmaktadır. 2026 yılı standartlarına göre SDL3 GPU ve Dear ImGui entegrasyonu temel alınmıştır.

---

## 1. Teknik Altyapı ve Teknoloji Yığını

### 1.1. Dear ImGui Entegrasyonu
*   **Platform Backend:** `imgui_impl_sdl3.cpp` (SDL3'ün gelişmiş windowing ve event sistemini kullanır).
*   **Renderer Backend:** `imgui_impl_sdlgpu3.cpp` (SDL3'ün modern GPU API'si ile native entegrasyon).
*   **Dil:** C++20.
*   **Docking:** "Multi-Viewport" ve "Docking" özellikleri aktif edilerek pencerelerin ana pencereden dışarı çıkarılabilmesi sağlanacak.

### 1.2. Render Lifecycle (İş Akışı)
SDL3 GPU mimarisinde ImGui çizimi şu sırayı takip eder:
1.  `ImGui_ImplSDL3_NewFrame()` & `ImGui::NewFrame()` çağrıları.
2.  Editör pencerelerinin kodlarının çalıştırılması (Hierarchy, Inspector vb.).
3.  `ImGui::Render()` ile çizim verilerinin oluşturulması.
4.  **Kritik Adım:** `SDL_BeginGPURenderPass` öncesinde `ImGui_ImplSDLGPU_PrepareDrawData()` çağrılarak vertex/index buffer'ların GPU'ya yüklenmesi.
5.  `SDL_BeginGPURenderPass` içinde `ImGui_ImplSDLGPU_RenderDrawData()` ile nihai çizim.

---

## 2. Editör Mimari Bileşenleri

### 2.1. EditorManager (Orkestratör)
Tüm editör sistemini yöneten merkezi birimdir.
*   **Sorumluluk:** ImGui init/shutdown, frame yönetimi, seçili entity (`SelectionContext`) takibi.
*   **Scene Viewport:** Oyun görüntüsünü bir `SDL_GPUTexture` olarak render edip ImGui penceresi içine `ImGui::Image` olarak gömer.

### 2.2. Inspector System (Bileşen Düzenleyici)
Varlıkların (Entity) özelliklerini anlık değiştirmeyi sağlar.
*   **Dynamic UI:** Entity üzerindeki her bir component (`CTransform`, `CLight`, `CMesh`) için özel widgetlar (`DragFloat3`, `ColorEdit3` vb.) oluşturur.
*   **Undo/Redo:** Komut deseni (Command Pattern) kullanılarak yapılan her değişikliğin geri alınabilmesi planlanmıştır.

### 2.3. Scene Hierarchy (Hiyerarşi Paneli)
Sahnede bulunan tüm entity'lerin ağaç yapısıdır.
*   **Parent-Child İlişkisi:** `CTransform` bileşenindeki hiyerarşiyi görselleştirir.
*   **Arama/Filtreleme:** İsim veya etiket (Tag) bazlı hızlı arama.

---

## 3. Görsel Düzen (Layout) ve UX Tasarımı

Editör, modern bir "Dark Theme" üzerine kurulacak ve şu yerleşimi takip edecektir:

### 3.1. Ana Yerleşim Planı
| Bölge | Fonksiyon | İçerik |
| :--- | :--- | :--- |
| **Üst Bar** | **Menu Bar** | File, Edit, Windows, Play/Stop/Pause Kontrolleri |
| **Sol Panel** | **Hierarchy** | Sahnedeki nesne listesi ve katman yönetimi |
| **Sağ Panel** | **Inspector** | Seçili nesnenin detayları, Component ekleme/çıkarma |
| **Alt Panel** | **Browser & Logs** | Proje dosyaları (Assets) ve Debug Konsolu (Tabs) |
| **Orta Alan** | **Viewport** | 3D Sahne görünümü (Gizmo kontrolleri ile birlikte) |

### 3.2. Viewport Etkileşimi (ImGuizmo)
*   **Gizmos:** Nesneleri mouse ile doğrudan sahne üzerinde hareket ettirmek, döndürmek ve ölçeklemek için `ImGuizmo` kütüphanesi kullanılacaktır.
*   **Camera Toggle:** Editör kamerası (Free Look) ve oyun kamerası arasında tek tuşla geçiş.

---

## 4. Dosya ve Veri Yönetimi

*   **Scene Serialization:** Editörde yapılan değişiklikler `.astral` (JSON tabanlı) formatında kaydedilecek.
*   **Asset Hot-Loading:** Bir doku veya model dosyası diskte değiştiğinde, editör bunu fark edip (File Watcher) motor içinde anlık güncelleyecektir.

---

## 5. Implementasyon Yol Haritası (Gelecek)
1.  `vcpkg` veya manuel olarak ImGui SDL3/GPU backend dosyalarının eklenmesi.
2.  `App.cpp` içinde temel ImGui başlatma kodunun yazılması.
3.  `RenderSystem`'in ana swapchain yerine bir `RenderTexture`'a çizim yapması için güncellenmesi (Viewport için).
4.  Bileşen bazlı Inspector pencerelerinin kodlanması.
