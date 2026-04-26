# src/ecs/entity.h

## Amacı ve Sorumlulukları
`Entity` sınıfı, oyun dünyasındaki her bir nesneyi temsil eder. Kendi başına bir işlevi yoktur; ona eklenen bileşenler (transform, shape) sayesinde anlam kazanır.

## Üyeler ve Metodlar
- `transform`: `CTransform` bileşeni.
- `shape`: `CShape` bileşeni.
- `isActive()`: Varlık hala yaşıyor mu?
- `destroy()`: Varlığı silinmek üzere işaretler.
- `id()`: Benzersiz kimlik numarası.
- `tag()`: Varlık türü (örn: "player", "enemy").

## İlişkiler
- **Include eder:** `src/ecs/components.h`.
- **Kullanılır:** `src/core/entity_manager.h` tarafından oluşturulur ve yönetilir.

## Kullanım Örneği
```cpp
auto e = entityManager.addEntity("enemy");
if (e->isActive()) {
    e->transform.pos.x += 10;
}
```
