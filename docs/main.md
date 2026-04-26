# games/pong/main.cpp

## Amacı ve Sorumlulukları
Projenin giriş noktasıdır. İşletim sisteminden gelen çağrıyı karşılar ve `App` sınıfını başlatarak kontrolü ona devreder.

## Üyeler ve Metodlar
- `main()`: Standart C++ giriş fonksiyonu. `App` nesnesi oluşturur ve yaşam döngüsünü tetikler.

## İlişkiler
- **Include eder:** `engine/core/app.h`, `common.h`.
- **Kullanılır:** İşletim sistemi tarafından çalıştırılır.

## Kullanım Örneği
(Bu dosya doğrudan giriş noktası olduğu için başka yerden çağrılmaz, ancak projenin nasıl ayağa kalktığını gösterir.)
```cpp
int main() {
    App app;
    app.init(...);
    app.run();
}
```
