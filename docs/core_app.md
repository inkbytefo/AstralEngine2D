# src/core/app.h / app.cpp

## Amacı ve Sorumlulukları
`App` sınıfı, oyunun ana motorudur. SDL penceresini açar, ana döngüyü (Main Loop) yönetir ve girdileri işler.

## Üyeler ve Metodlar
- `init()`: SDL ve oyun sistemlerini başlatır.
- `run()`: `processEvents`, `update`, `render` üçlüsünü sürekli çalıştıran döngüdür.
- `processEvents()`: Klavye ve sistem olaylarını yakalar.
- `update()`: Mantıksal güncellemeleri (hareket, çarpışma vb.) yapar.
- `render()`: `EntityManager`'daki varlıkları ekrana çizer.

## İlişkiler
- **Include eder:** `SDL3/SDL.h`, `src/core/entity_manager.h`.
- **Kullanılır:** `main` fonksiyonu (`src/main.cpp`) tarafından başlatılır.

## Kullanım Örneği
```cpp
App game;
game.init("My Game", 800, 600);
game.run();
```
