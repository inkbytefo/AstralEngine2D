# engine/core/entity_manager.h / entity_manager.cpp

## Amacı ve Sorumlulukları
`EntityManager`, tüm varlıkların yaşam döngüsünü yöneten "fabrikadır". Varlıkların oluşturulması, listelenmesi ve silinmesi bu sınıfın sorumluluğundadır. Gecikmeli ekleme/silme (delayed dispatch) yaparak döngü güvenliğini sağlar.

## Üyeler ve Metodlar
- `update()`: Silinmesi gerekenleri temizler, bekleyenleri ekler. Her karede bir kez çağrılmalıdır.
- `addEntity(tag)`: Yeni bir varlık kuyruğa ekler.
- `getEntities()`: Tüm aktif varlıkları döndürür.
- `getEntities(tag)`: Belirli bir gruptaki varlıkları döndürür.

## İlişkiler
- **Include eder:** `engine/ecs/entity.h`, `<vector>`, `<map>`, `<memory>`.
- **Kullanılır:** `engine/core/app.h` sınıfı tarafından oyun dünyasını yönetmek için kullanılır.

## Kullanım Örneği
```cpp
EntityManager manager;
manager.addEntity("bullet");
manager.update(); // Mermiyi aktif listeye alır.
auto& enemies = manager.getEntities("enemy");
```
