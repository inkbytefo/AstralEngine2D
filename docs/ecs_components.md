# src/ecs/components.h

## Amacı ve Sorumlulukları
Bu dosya, varlıkların (Entity) sahip olabileceği veri paketlerini (Bileşenler) tanımlar. ECS (Entity-Component-System) mimarisinin "Bileşen" kısmını temsil eder. Sadece veri tutarlar, mantık içermezler.

## Yapılar (Structs)
### CTransform
Varlığın dünyadaki fiziksel durumunu tutar.
- `Vec2 pos`: Mevcut konum.
- `Vec2 velocity`: Mevcut hız.

### CShape
Varlığın nasıl çizileceğini tanımlar.
- `float width, height`: Boyutlar.
- `uint8_t r, g, b, a`: Renk ve şeffaflık (0-255).

## İlişkiler
- **Include eder:** `src/math/vec2.h`.
- **Kullanılır:** `src/ecs/entity.h` içinde varlık üyeleri olarak yer alırlar.

## Kullanım Örneği
```cpp
CTransform transform({400, 300}, {0, 0});
CShape shape(50, 50, 255, 0, 0, 255); // Kırmızı kare
```
