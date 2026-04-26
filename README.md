# Astral Engine 2D

Astral Engine 2D, C++ ve SDL3 kütüphanesi kullanılarak geliştirilmiş, hafif ve modüler bir 2D oyun motoru iskeletidir. Veri odaklı tasarım (Data-Oriented Design) ve basitleştirilmiş bir ECS (Entity-Component-System) mimarisini temel alır.

## 🚀 Özellikler

- **SDL3 Entegrasyonu**: En güncel SDL3 özelliklerini kullanır.
- **ECS Mimarisi**: Varlık (Entity) ve Bileşen (Component) yönetimi ile modüler yapı.
- **Hızlı Matematik**: 2D vektör işlemleri için optimize edilmiş `Vec2` kütüphanesi.
- **Gecikmeli İşlem (Delayed Dispatch)**: Oyun döngüsü sırasında güvenli varlık ekleme ve silme.
- **Modern C++**: C++20 standartları ile geliştirilmiştir.

## 📁 Proje Yapısı

- `src/core/`: Motorun ana çekirdek sınıfları (App, EntityManager).
- `src/ecs/`: Varlık ve Bileşen tanımlamaları.
- `src/math/`: Matematik yardımcıları.
- `docs/`: Detaylı teknik dokümantasyon.
- `assets/`: Oyun kaynakları (görseller, sesler).

## 🛠️ Kurulum

### Gereksinimler
- **CMake** (v3.10+)
- **SDL3**
- **C++20** destekleyen bir derleyici (MSVC, GCC veya Clang)

### Derleme

1. Depoyu klonlayın:
   ```bash
   git clone https://github.com/inkbytefo/AstralEngine2D.git
   ```
2. Proje dizinine gidin ve bir build klasörü oluşturun:
   ```bash
   mkdir build
   cd build
   ```
3. CMake ile yapılandırın ve derleyin:
   ```bash
   cmake ..
   cmake --build .
   ```

## 📚 Dokümantasyon

Daha fazla detay için [docs/OVERVIEW.md](docs/OVERVIEW.md) dosyasını inceleyebilirsiniz.

## ⚖️ Lisans ve Teşekkür

- **Astral Engine 2D**: Bu proje [MIT Lisansı](LICENSE) altında korunmaktadır.
- **SDL3**: Bu proje [zlib lisansı](https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt) ile lisanslanmış olan SDL3 kütüphanesini kullanmaktadır. SDL ekibine bu harika kütüphane için teşekkür ederiz.

## 📄 Lisans

Bu proje MIT lisansı ile lisanslanmıştır. Daha fazla bilgi için `LICENSE` dosyasına bakınız.
