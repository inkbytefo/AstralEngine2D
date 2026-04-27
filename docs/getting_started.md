# Astral Engine İle Oyun Geliştirmeye Başlamak

Motorun (Engine) kaynak koduna hiç dokunmadan, sadece `games/` klasörünüz altında kendi oyununuzun mantığını (Logic) nasıl yazacağınızı bu rehberde bulabilirsiniz.

## Adım 1: Sahneleri (Scenes) Anlamak
Her şey bir Sahne (`Scene`) içinde gerçekleşir. Ana menünüz bir sahnedir, oyunun ilk bölümü (Level 1) başka bir sahnedir.
Yeni bir sahne yaratmak için `Scene` sınıfından miras alan bir C++ sınıfı oluşturmalısınız (Örn: `SandboxScene`).

Sahne açıldığında çalışacak kodlar `init()` fonksiyonuna yazılır:
```cpp
void SandboxScene::init() {
    // 1. Oyuncu karakterini oluştur
    auto player = m_entityManager.addEntity("player");
    player->add<CTransform>(glm::vec3(0, 0, 0), glm::vec3(0));
    player->add<CMesh>("knight_mesh", "knight_material");
    
    // Sizin yarattığınız özel bir bileşeni ekleyin
    player->add<CHealth>(100); 
}
```

## Adım 2: Özel Bileşenler (Components) Yaratmak
Oyununuzun ihtiyacı olan verileri `components.h` içerisinde struct olarak tanımlayın. Unutmayın, bileşenlerin içinde fonksiyon (logic) olmaz, sadece veri olur!

```cpp
// Sadece sizin oyununuzda var olacak bir bileşen
struct CHealth {
    bool has{ false };
    int currentHp{ 100 };
    int maxHp{ 100 };
    
    CHealth(int hp) : currentHp(hp), maxHp(hp) {}
};
```

*(Not: Yeni bir bileşen yarattıktan sonra onu `entity.h` içindeki `ComponentTuple` listesine eklemeyi unutmayın).*

## Adım 3: Sistemler (Systems) ile Oyun Mantığını Yazmak
Verileri işlemek için Sistemleri kullanırız. Örneğin bir `CombatSystem::update()` fonksiyonu yazıp, her karede (frame) tüm düşmanların canını kontrol edebilirsiniz.

```cpp
void CombatSystem::update(EntityManager& em) {
    // Tüm varlıkları gez
    for (auto& entity : em.getEntities()) {
        
        // Eğer bu varlıkta hem Sağlık hem de Hasar Alma bileşeni varsa
        if (entity->has<CHealth>() && entity->has<CDamageTaken>()) {
            
            // Hasarı canından düş
            int damage = entity->get<CDamageTaken>().amount;
            entity->get<CHealth>().currentHp -= damage;
            
            // Varlık öldü mü?
            if (entity->get<CHealth>().currentHp <= 0) {
                entity->destroy(); // ECS onu otomatik olarak sahneden silecek
            }
            
            // İşlenen hasar bileşenini kaldır (sürekli aynı hasarı almasın diye)
            entity->remove<CDamageTaken>();
        }
    }
}
```

Hazırladığınız sistemleri sahnenizin `update(float deltaTime)` döngüsü içerisinde çağırırsanız, oyununuzun tüm mantığı tıkır tıkır işlemeye başlayacaktır.

## Özet Akış
1. `init()` içinde objeleri (Entity) yarat.
2. Objelerin üzerine sadece verilerini (Component) yerleştir.
3. `update()` döngüsünde bu verileri alıp değiştiren kuralları (System) çalıştır.
4. Render motoru (Zaten arkada çalışıyor) değişen pozisyonlara göre objeleri yeni yerlerinde ekrana çizsin.
