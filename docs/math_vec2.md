# src/math/vec2.h

## Amacı ve Sorumlulukları
`Vec2` yapısı, oyun dünyasındaki 2 boyutlu vektör işlemlerini (konum, hız, ivme vb.) basitleştirmek için tasarlanmıştır. Matematiksel işlemleri operatör aşırı yüklemeleri (operator overloading) ile doğal bir şekilde yapmayı sağlar.

## Üyeler ve Metodlar
- `float x, y`: Vektörün bileşenleri.
- `operator+`, `operator-`: Vektörel toplama ve çıkarma.
- `operator*`: Skaler ile çarpma (vektörü ölçeklendirme).
- `operator+=`, `operator-=`: Mevcut vektörü yerinde güncelleme.

## İlişkiler
- **Include eder:** Yok.
- **Kullanılır:** `components.h`, `app.h`, `entity.h` ve projenin hemen her yerinde temel veri tipi olarak kullanılır.

## Kullanım Örneği
```cpp
Vec2 konum(100, 100);
Vec2 hiz(5, 0);
konum += hiz * 2.0f; // Konumu 110, 100 yapar.
```
