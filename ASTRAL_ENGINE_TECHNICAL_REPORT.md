# Astral Engine Teknik Inceleme ve Refaktor Roadmap

**Tarih:** 27 Nisan 2026  
**Hazirlayan:** Kilo  
**Proje:** Astral Engine  
**Dil:** C++20

---

## Ozet

Proje su anda SDL3 GPU tabanli, editor entegreli, ECS odakli erken asama bir 3D oyun motoru durumunda. Temel yon dogru secilmis:

- `engine/` ve `games/` ayrimi net
- `App -> Scene -> SystemManager -> Renderer/Editor` akis modeli anlasilir
- Asset yonetimi registry/manager ayrimi ile genislemeye uygun
- SDL3 GPU, PBR shader, glTF import ve editor viewport hedefleri tutarli

Kod tabani calisabilir durumda, ancak buyumeden once cekirdek mimaride birkac kritik duzenleme yapilmasi gerekiyor.

---

## Mevcut Mimari

### Katmanlar

- `engine/core`: uygulama omurgasi, scene, entity/system yonetimi, asset registry
- `engine/ecs`: entity, component ve trait modeli
- `engine/systems`: input, transform, camera, physics, render, trait
- `engine/renderer`: SDL3 GPU abstraction ve render backend
- `engine/editor`: ImGui tabanli editor ve paneller
- `games/sandbox`: motoru kullanan ornek scene

### Ana Akis

1. `games/sandbox/main.cpp` uygulamayi baslatir.
2. `App` SDL, GPU, editor ve sistemleri ayaga kaldirir.
3. `Scene` kendi entity dunyasini kurar.
4. `SystemManager` sistemleri priority sirasiyla calistirir.
5. `RenderSystem` entity verisini renderer uzerinden GPU'ya cizer.
6. `EditorManager` viewport ve panel UI'sini sahne texture'i ustunde sunar.

### Mimari Karakteri

Bugunku haliyle motoru en dogru tanimlayan ifade su:

> SDL3 GPU tabanli, editor entegreli, compile-time component tuple kullanan, scene ve sandbox odakli erken asama bir 3D ECS oyun motoru.

---

## Guclu Yonler

- `engine` ve `games` ayrimi uzun vadede dogru
- `SystemManager` ile update/event orchestration sade
- `AssetRegistry` ve alt manager'lar temiz bir genisleme noktasi sunuyor
- `RenderSystem` 3D/PBR yonunu netlestiriyor
- `EditorManager` docking, hierarchy, inspector ve viewport temelini atmis
- `SceneSerializer` ile play/edit snapshot mantigi dusunulmus
- `Trait` sistemi ile script-benzeri davranislar icin zemin var

---

## Tespit Edilen Zayifliklar

### 1. ECS belgede anlatilandan daha basit

Dokumantasyonda DOD/ECS vurgusu guclu, fakat implementasyon gercekte:

- `shared_ptr<Entity>` tabanli
- entity icinde tuple component saklayan
- runtime filtreleme yapan bir yapi

Bu kotu degil, ama gercek bir SoA/archetype ECS degil. Beklenti ile implementasyon arasinda fark var.

### 2. `EntityManager::view()` frame bazli allocation uretiyor

Her cagrida yeni `EntityVec` olusturuluyor. Ustune `shrink_to_fit()` cagrisi heap churn olusturabilir.

### 3. `App` sinifi fazla sorumluluk yuklenmis

Tek sinif su anda:

- SDL init/shutdown
- GPU init
- ses sistemi init
- sistem kaydi
- scene switching
- editor panel lifecycle
- play/edit state gecisleri
- snapshot yonetimi
- ana loop
- render orchestration

Bu buyudugunde bakimi zorlastirir.

### 4. Runtime/editor/state gecisleri daginik

Play, pause, stop ve snapshot davranisi birden fazla yerde dagitilmis:

- `App`
- `EditorManager`
- `ViewportPanel`

Bu state machine tek elde toplanmali.

### 5. Kamera akisi yeterince soyut degil

`RenderSystem` editor override ile calisiyor. Gameplay camera ve editor camera icin net bir ortak kontrat yok.

### 6. Render batching var ama gercek instancing yok

Benzer objeler gruplanmis, fakat cizim hala tek tek draw call ile yapiliyor.

### 7. Light sistemi render'a tam bagli degil

`CLight` mevcut, shader uniform yapisi hazir, fakat render tarafinda aktif isik toplama ve shader'a gecirme tam degil.

### 8. GLTF loader header-only tasarlanmis

`gltf_loader.h` icinde cok fazla implementasyon var. Bu derleme suresini ve bagimlilik yogunlugunu arttiriyor.

### 9. Dokumantasyon geride kalmis

- README hala agirlikli olarak 2D anlatiyor
- klasor isimleri guncel degil
- editor ve 3D/PBR karakteri tam yansimiyor

### 10. Test kapsami cekirdek davranislari guvencelemiyor

Ozellikle eksik alanlar:

- scene serializer
- transform hierarchy
- trait lifecycle
- camera secimi
- asset fallback davranisi

---

## Onerilen Refaktor Yonu

### Kisa Vade

- `App` icindeki runtime/editor/state sorumluluklarini ayir
- play/pause/stop/snapshot akislarini tek merkezde topla
- camera source mantigini soyutla
- `view()` allocation maliyetini azalt

### Orta Vade

- render path'i netlestir: aktif kamera, isik toplama, instancing
- GLTF importu `.cpp` tabanli hale getir
- editor panel lifecycle ve scene binding'i sadeleştir

### Uzun Vade

- query cache veya daha guclu ECS gecisi
- asset handle sistemi
- hot reload ve async loading
- gizmo, prefab, undo/redo, scene tooling

---

## Yol Haritasi

Bu roadmap, refaktor islerini birbirine risk yaratmadan sirali sekilde uygulamak icin hazirlandi.

### Faz 1: Runtime ve State Ayrisma

Hedef: `App` icindeki fazla sorumlulugu azaltmak.

Gorevler:

1. Play/Edit/Pause state gecislerini tek bir runtime controller'a tasimak
2. Scene snapshot alma ve restore sorumlulugunu panel kodundan cikarmak
3. Scene yuklenince editor panel olusturma ile panel-scene binding davranisini ayirmak
4. `App::run()` icindeki ana donguyu okunur alt adimlara bolmek

Beklenen sonuc:

- `App` daha ince bir orchestration sinifi olur
- editor UI ile simulation state mantigi ayrisir

### Faz 2: Kamera Akisi ve Render Girisleri

Hedef: render sisteminin aktif kamerayi temiz bir kontratla almasi.

Gorevler:

1. Ortak bir `CameraFrameData` veya benzeri veri yapisi tanimlamak
2. Editor kamerasi ve gameplay kamerasi icin ayni cikis modelini kullanmak
3. `RenderSystem` icindeki override mantigini sadelestirmek
4. Kamera secim politikasini netlestirmek: editor override, active scene camera fallback

Beklenen sonuc:

- render sistemi daha az ozel durumla calisir
- editor/runtime kamera davranislari daha anlasilir olur

### Faz 3: ECS ve Query Iyilestirmeleri

Hedef: mevcut ECS yapisini buyumeye biraz daha uygun hale getirmek.

Gorevler:

1. `EntityManager::view()` icindeki gereksiz `shrink_to_fit()` kullanimini kaldirmak
2. sik kullanilan query'ler icin reusable traversal mantigi dusunmek
3. API semantigini netlestirmek: `addEntity()` sonrasi gorunurluk ne zaman garanti edilir
4. dokumantasyonda ECS'in gercek dogasini acik yazmak

Beklenen sonuc:

- frame bazli gereksiz allocation azalir
- beklenti/implementasyon uyumu artar

Durum:

- `view()` icindeki gereksiz kapasite daraltma davranisi kaldirildi
- pending entity semantigi API seviyesinde `getPendingEntities()` ile gorunur hale getirildi
- testler deferred activation davranisiyla uyumlu hale getirildi
- allocation uretmeyen `each<T...>(callback)` traversal API'si eklendi
- sicak yol kullanan temel sistemler `view()` yerine `each()` kullanacak sekilde iyilestirilmeye baslandi
- `RenderSystem` query ve scene camera resolve yollari `each()` tabanina tasindi
- tag lookup icin `tryGetEntities()` eklendi; eksik tag sorgulari artik map'e sessiz insertion yapmadan kontrol edilebiliyor
- Faz 3 kapsamindaki entity lifecycle ve query davranislari testlerle guvence altina alinmaya baslandi

### Faz 4: Render Dogrulugu

Hedef: gorsel boruyu davranissal olarak tamamlamak.

Gorevler:

1. aktif scene light'larini toplayip shader'a gecirmek
2. model matrisinde `globalMatrix` kullanimini netlestirmek
3. render batch sistemini gercek instancing'e yaklastirmak
4. fallback material/pipeline davranislarini sertlestirmek

Beklenen sonuc:

- render daha dogru ve daha olceklenebilir hale gelir

### Faz 5: Asset ve Import Katmani

Hedef: asset ve import kodunu derli toplu hale getirmek.

Gorevler:

1. `GLTFLoader` implementasyonunu `.cpp` dosyasina tasimak
2. asset lookup ve fallback mantigini daha guvenli hale getirmek
3. asset manager naming ve ownership semantiklerini netlestirmek

### Faz 6: Test ve Dokumantasyon

Hedef: refaktorleri guvence altina almak.

Gorevler:

1. serializer ve hierarchy testleri eklemek
2. trait lifecycle testleri eklemek
3. camera secimi ve runtime state testleri eklemek
4. README ve `docs/architecture.md` dosyalarini guncellemek

---

## Uygulama Stratejisi

Refaktorleri tek seferde buyuk bir kirilimla yapmak yerine, her fazi asagidaki prensiplerle uygulamak daha dogru:

- once davranisi koru, sonra sorumluluklari ayir
- minimal degisikliklerle ilerle
- her adimdan sonra build/test dogrulamasi yap
- editor/runtime/render ayrimini arttir, ama mevcut kullanimlari bozma

---

## Ilk Uygulanacak Isler

Ilk dalgada uygulanmasi gerekenler:

1. Runtime scene state ve snapshot akislarini tek bir merkezde toplamak
2. `ViewportPanel` icindeki play/stop snapshot sorumlulugunu tasimak
3. `App` icindeki scene activation ve per-frame orchestration'i alt fonksiyonlara ayirmak
4. Kamera verisini `RenderSystem` icin daha temiz bir veri modeliyle toplamak
5. `EntityManager::view()` icindeki gereksiz kapasite daraltma davranisini kaldirmak

---

## Sonuc

Motorun temeli saglam ve yonu dogru. En buyuk ihtiyac yeni ozellik eklemekten once cekirdek akislarin sadeleştirilmesi. Su an yapilacak kontrollu refaktorler, ileride editor, render ve gameplay tarafinda birikerek buyuyecek karmasikligi ciddi bicimde azaltir.
