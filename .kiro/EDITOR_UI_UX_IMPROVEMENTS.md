# 🎨 Astral Engine Editor - UI/UX İyileştirme Önerileri

## 📊 Mevcut Durum Analizi
- ✅ Modüler panel sistemi (SceneHierarchy, Properties, Viewport)
- ✅ ImGuizmo gizmo mode seçimi (T/R/S)
- ✅ Viewport bilgi paneli (FPS, Draw Calls, Aspect Ratio)
- ❌ Renk şeması profesyonel değil (ImGui default dark)
- ❌ Panel başlıkları ve ikonlar eksik
- ❌ Toolbar ve menu bar minimal
- ❌ Viewport gizmo göstergeleri (mode indicator) eksik
- ❌ Entity seçim highlight'ı zayıf
- ❌ Dockspace layout'u kaydedilmiyor

---

## 🎯 FAZE 1: Renk Şeması & Tema (1 saat)

### Unity-Tarzı Tema (Önerilen)
```
Primary Colors:
- Background: #1E1E1E (Koyu gri)
- Panel BG: #252526 (Biraz daha açık)
- Accent: #0E639C (Mavi - Unity tarzı)
- Highlight: #007ACC (Parlak mavi)
- Text: #CCCCCC (Açık gri)
- Success: #4EC9B0 (Turkuaz)
- Warning: #CE9178 (Turuncu)
- Error: #F48771 (Kırmızı)
```

### Unreal-Tarzı Tema (Alternatif)
```
Primary Colors:
- Background: #1A1A1A (Çok koyu)
- Panel BG: #2D2D2D (Koyu gri)
- Accent: #00D4FF (Cyan - Unreal tarzı)
- Highlight: #00FFFF (Parlak cyan)
- Text: #E0E0E0 (Açık gri)
- Success: #00FF00 (Yeşil)
- Warning: #FFAA00 (Turuncu)
- Error: #FF0000 (Kırmızı)
```

### ImGui Stil Ayarları
```cpp
// Unity-tarzı tema
ImGuiStyle& style = ImGui::GetStyle();
style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
style.Colors[ImGuiCol_Button] = ImVec4(0.06f, 0.39f, 0.61f, 1.0f);
style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.48f, 0.80f, 1.0f);
style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.40f, 0.70f, 1.0f);
style.Colors[ImGuiCol_Header] = ImVec4(0.06f, 0.39f, 0.61f, 0.4f);
style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.06f, 0.39f, 0.61f, 0.6f);
style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.39f, 0.61f, 0.8f);

// Padding ve rounding
style.FramePadding = ImVec2(6, 4);
style.FrameRounding = 3.0f;
style.WindowRounding = 4.0f;
style.GrabRounding = 3.0f;
```

---

## 🎯 FAZE 2: Panel Başlıkları & İkonlar (30 dakika)

### Hierarchy Panel
```
🌳 Hierarchy
├─ 📦 Entity 1
├─ 📦 Entity 2
└─ 📦 Entity 3
```

### Properties Panel
```
🔧 Inspector
├─ 📍 Transform
├─ 🎥 Camera
├─ 💡 Light
└─ 🎨 Material
```

### Viewport Panel
```
👁️ Viewport [1280x720] [16:9] | FPS: 60 | Draw Calls: 42
```

### İkon Sistemi
```cpp
// Unicode karakterler kullanarak basit ikonlar
const char* ICON_ENTITY = "📦";
const char* ICON_CAMERA = "🎥";
const char* ICON_LIGHT = "💡";
const char* ICON_MESH = "🔺";
const char* ICON_TRANSFORM = "📍";
const char* ICON_HIERARCHY = "🌳";
const char* ICON_PROPERTIES = "🔧";
const char* ICON_VIEWPORT = "👁️";
```

---

## 🎯 FAZE 3: Toolbar & Menu Bar (45 dakika)

### Top Toolbar
```
[File] [Edit] [View] [Tools] [Help]
  |
  ├─ File
  │  ├─ New Scene
  │  ├─ Open Scene
  │  ├─ Save Scene
  │  └─ Exit
  │
  ├─ Edit
  │  ├─ Undo (Ctrl+Z)
  │  ├─ Redo (Ctrl+Y)
  │  ├─ Delete (Del)
  │  └─ Duplicate (Ctrl+D)
  │
  ├─ View
  │  ├─ Reset Layout
  │  ├─ Fullscreen Viewport
  │  └─ Show Grid
  │
  └─ Tools
     ├─ Play (Space)
     ├─ Pause
     └─ Stop
```

### Viewport Toolbar (Viewport panelinin üstünde)
```
[T] [R] [S] | [Grid] [Gizmo] [Wireframe] | [Cam Speed: ▼]
```

---

## 🎯 FAZE 4: Viewport Geliştirmeleri (1 saat)

### Gizmo Mode Indicator
```cpp
// Viewport'un sol üst köşesinde
"Gizmo Mode: TRANSLATE (T)" // Renkli badge
"Gizmo Mode: ROTATE (R)"     // Renkli badge
"Gizmo Mode: SCALE (S)"      // Renkli badge
```

### Grid & Gizmo Ayarları
```
☑ Show Grid
☑ Show Gizmo
☑ Show Wireframe
☑ Show Normals
☑ Show Bounds
```

### Viewport Kamera Bilgisi
```
Camera: Main Camera
Position: (10.0, 0.0, 2.0)
Rotation: (0°, 0°, 0°)
FOV: 60°
```

---

## 🎯 FAZE 5: Hierarchy Panel Iyileştirmeleri (45 dakika)

### Entity Seçim Highlight'ı
```cpp
// Seçili entity'nin arka planı parlak mavi
// Hover entity'nin arka planı hafif gri
// Parent entity'nin arka planı daha açık
```

### Context Menu İkonları
```
📦 Create Empty
📦 Create Cube
📦 Create Sphere
🗑️ Delete
📋 Duplicate
🔗 Rename
```

### Drag-Drop Göstergesi
```
Entity1
├─ Entity2 (Drag over: "Drop here to parent")
└─ Entity3
```

---

## 🎯 FAZE 6: Properties Panel Iyileştirmeleri (1 saat)

### Component Başlıkları
```
✓ Transform (Collapsible header)
  📍 Position: [X] [Y] [Z]
  🔄 Rotation: [X] [Y] [Z]
  📏 Scale: [X] [Y] [Z]

✓ Camera (Collapsible header)
  🎥 Active: ☑
  📐 FOV: [60]
  📏 Aspect: [16:9]

✓ Light (Collapsible header)
  💡 Type: [Directional ▼]
  🎨 Color: [████]
  ⚡ Intensity: [1.0]
```

### Değer Düzenleme
```
- Slider ile hızlı ayarlama
- Input field ile kesin değer
- Reset butonu (varsayılan değere dön)
- Copy/Paste değer
```

---

## 🎯 FAZE 7: Status Bar (30 dakika)

### Bottom Status Bar
```
┌─────────────────────────────────────────────────────────────┐
│ 📊 Entities: 42 | 🎥 Cameras: 1 | 💡 Lights: 3 | 🔺 Meshes: 15 │
│ 🎮 Play Mode: OFF | 🔒 Lock: OFF | 📐 Snap: OFF              │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 Uygulama Sırası (Önerilen)

### Hafta 1:
1. **FAZE 1** - Renk şeması (1 saat)
2. **FAZE 2** - Panel başlıkları & ikonlar (30 dakika)
3. **FAZE 3** - Toolbar & menu bar (45 dakika)

### Hafta 2:
4. **FAZE 4** - Viewport geliştirmeleri (1 saat)
5. **FAZE 5** - Hierarchy panel iyileştirmeleri (45 dakika)
6. **FAZE 6** - Properties panel iyileştirmeleri (1 saat)
7. **FAZE 7** - Status bar (30 dakika)

---

## 🎨 Renk Paletleri (Hex Kodları)

### Unity-Tarzı (Önerilen)
```
#1E1E1E - Background
#252526 - Panel BG
#3E3E42 - Hover
#0E639C - Accent (Mavi)
#007ACC - Highlight (Parlak Mavi)
#CCCCCC - Text
#4EC9B0 - Success (Turkuaz)
#CE9178 - Warning (Turuncu)
#F48771 - Error (Kırmızı)
```

### Unreal-Tarzı (Alternatif)
```
#1A1A1A - Background
#2D2D2D - Panel BG
#3D3D3D - Hover
#00D4FF - Accent (Cyan)
#00FFFF - Highlight (Parlak Cyan)
#E0E0E0 - Text
#00FF00 - Success (Yeşil)
#FFAA00 - Warning (Turuncu)
#FF0000 - Error (Kırmızı)
```

---

## 💡 Ek Öneriler

### Accessibility
- ✅ Yüksek contrast renk şeması
- ✅ Keyboard shortcuts (T/R/S, Ctrl+Z, Del, vb.)
- ✅ Tooltip'ler tüm butonlarda
- ✅ Font boyutu ayarlanabilir

### Performance
- ✅ Lazy rendering (sadece değişen paneller render edilsin)
- ✅ Viewport'u minimize ettiğinde render etme
- ✅ Panel collapse/expand animasyonları smooth

### Consistency
- ✅ Tüm panellerde aynı stil
- ✅ Aynı ikonlar, aynı renkler
- ✅ Aynı padding, rounding, spacing

---

## 🚀 Başlama Adımları

1. **EditorManager'da ImGui stil ayarlarını ekle**
2. **Panel başlıklarına ikonlar ekle**
3. **Toolbar ve menu bar oluştur**
4. **Viewport'a gizmo mode indicator ekle**
5. **Hierarchy panel'e highlight ve ikonlar ekle**
6. **Properties panel'i component başlıklarıyla düzenle**
7. **Status bar ekle**

---

## 📝 Notlar

- Mevcut sistem **SAKLANACAK** - sadece görsel iyileştirmeler
- Tüm değişiklikler **backward compatible** olacak
- Renk şeması **tema olarak kaydedilebilir** (gelecekte)
- İkonlar **Unicode karakterler** kullanacak (font yüklemeye gerek yok)

