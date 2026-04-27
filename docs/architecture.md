# Astral Engine Mimarisi (Architecture)

Astral Engine, performans, modülerlik ve genişletilebilirlik ilkeleri üzerine inşa edilmiş modern bir oyun motorudur. Çekirdek mimarisi olarak **Entity-Component-System (ECS)** ve **Data-Oriented Design (DOD)** prensiplerini benimser.

## 1. Klasör ve Proje Yapısı

Motorun kod tabanı, "motor çekirdeği" ve "oyun kodu" birbirine karışmayacak şekilde iki ana klasöre ayrılmıştır:

- `engine/`: Oyunun türünden bağımsız olan çekirdek sistemleri (Render, Ses, ECS, Fizik) barındırır.
- `games/`: Motoru kullanarak geliştirilen oyun projelerini (örn. `sandbox`) içerir. Tüm oyun mantığı burada bulunur.

Bu yapı sayesinde aynı motor kodunu kullanarak birden fazla farklı oyun geliştirebilirsiniz.

## 2. Entity-Component-System (ECS)

Geleneksel Nesne Yönelimli Programlamada (OOP) kullanılan derin kalıtım hiyerarşilerinin (örn: `Entity -> Character -> Enemy -> Orc`) aksine, Astral Engine kompozisyon (composition) mantığıyla çalışır.

### Entity (Varlık)
Entity'ler sadece benzersiz bir kimlik (ID) ve bir etiketten (Tag) ibarettir (`entity.h`). Kendi içlerinde hiçbir mantık veya veri barındırmazlar. Onları birer "konteyner" veya "kimlik numarası" olarak düşünebilirsiniz.

### Component (Bileşen)
Bileşenler sadece veriden (Data) ibarettir, içlerinde fonksiyon barındırmazlar (`components.h`).
Örnek bileşenler:
- `CTransform`: Pozisyon, boyut ve döndürme verisini tutar.
- `CLight`: Işığın rengini, şiddetini ve yönünü tutar.
- `CMesh`: 3D model ve materyal bilgisini tutar.

### System (Sistem)
Sistemler sadece mantıktan (Logic) ibarettir, içlerinde kalıcı durum (State) barındırmazlar. Belirli bileşenlere sahip olan Entity'leri bulur ve verilerini işlerler.
- `RenderSystem`: `CTransform` ve `CMesh` bileşeni olan tüm varlıkları bulur ve ekrana çizer.
- `TransformSystem`: `CTransform` içerisindeki Scene Graph (ebeveyn-çocuk) hiyerarşisini çözerek global matrisleri hesaplar.

## 3. Data-Oriented Design (Veri Odaklı Tasarım)
Astral Engine'de bileşenler hafızada (memory) bitişik diziler halinde (contiguous arrays) tutulmaya çalışılır. Bu yaklaşım:
1. **Cache Miss** (Önbellek ıskalaması) oranını düşürür.
2. İşlemcinin (CPU) verileri toplu olarak işlemesini (SIMD) kolaylaştırır.
3. Çoklu iş parçacığı (Multi-threading) mimarisine geçişi destekler.

## 4. Scene Graph (Transform Hiyerarşisi)
Her ECS varlığı `CTransform` bileşeni üzerinden bir `parent` (ebeveyn) ve birden fazla `children` (çocuk) barındırabilir.
`TransformSystem`, her karede bu ilişkileri okuyarak nesnelerin Dünya Koordinatlarındaki (Global Space) yerini hesaplar.
Böylece; bir karakterin eline bağlanan kılıç, karakter yürüdüğünde otomatik olarak onunla birlikte hareket eder.
