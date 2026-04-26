#pragma once
#include <SDL3_mixer/SDL_mixer.h>
#include <map>
#include <string>
#include <SDL3/SDL.h>

class SoundManager {
public:
    static SoundManager& getInstance() {
        static SoundManager instance;
        return instance;
    }

    bool init() {
        if (!MIX_Init()) {
            SDL_Log("SDL_mixer baslatilamadi: %s", SDL_GetError());
            return false;
        }

        // SDL3 Mixer'da varsayılan cihazı aç
        m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!m_mixer) {
            SDL_Log("Ses mikseri olusturulamadi: %s", SDL_GetError());
            return false;
        }

        // Müzik için ayrı bir track (kanal) oluşturalım
        m_musicTrack = MIX_CreateTrack(m_mixer);
        if (!m_musicTrack) {
            SDL_Log("Muzik track'i olusturulamadi: %s", SDL_GetError());
        }

        return true;
    }

    // Efekt Yükle (WAV, MP3, OGG)
    bool loadSound(const std::string& name, const std::string& path) {
        if (!m_mixer) return false;
        
        // true: Sesi belleğe tam olarak açar (kısa efektler için hızlı çalışır)
        MIX_Audio* audio = MIX_LoadAudio(m_mixer, path.c_str(), true);
        if (!audio) {
            SDL_Log("Ses yuklenemedi: %s - %s", path.c_str(), SDL_GetError());
            return false;
        }
        m_sounds[name] = audio;
        return true;
    }

    // Müzik Yükle (Arka Plan)
    bool loadMusic(const std::string& name, const std::string& path) {
        if (!m_mixer) return false;
        
        // false: Sesi anında çözmez (predecode), çalarken parça parça çözer (uzun müzikler için)
        MIX_Audio* audio = MIX_LoadAudio(m_mixer, path.c_str(), false);
        if (!audio) {
            SDL_Log("Muzik yuklenemedi: %s - %s", path.c_str(), SDL_GetError());
            return false;
        }
        m_musics[name] = audio;
        return true;
    }

    // Efekt Çal
    void playSound(const std::string& name) {
        if (m_mixer && m_sounds.count(name)) {
            // MIX_PlayAudio: Geçici track oluşturarak efekti bir kez çalar
            MIX_PlayAudio(m_mixer, m_sounds[name]); 
        }
    }

    // Müzik Çal
    void playMusic(const std::string& name) {
        if (m_mixer && m_musicTrack && m_musics.count(name)) {
            // Müziği track'e ata
            MIX_SetTrackAudio(m_musicTrack, m_musics[name]);
            
            // Özellik oluştur ve sonsuz döngü ayarla
            SDL_PropertiesID props = SDL_CreateProperties();
            SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
            
            // Çalmayı başlat
            MIX_PlayTrack(m_musicTrack, props);
            
            SDL_DestroyProperties(props);
        }
    }

    void stopMusic() {
        if (m_musicTrack) {
            // 0 ms fade-out (anında durdur)
            MIX_StopTrack(m_musicTrack, 0);
        }
    }

    void cleanup() {
        for (auto& [name, audio] : m_sounds) MIX_DestroyAudio(audio);
        m_sounds.clear();

        for (auto& [name, audio] : m_musics) MIX_DestroyAudio(audio);
        m_musics.clear();

        if (m_musicTrack) {
            MIX_DestroyTrack(m_musicTrack);
            m_musicTrack = nullptr;
        }

        if (m_mixer) {
            MIX_DestroyMixer(m_mixer);
            m_mixer = nullptr;
        }
        
        MIX_Quit();
    }

private:
    SoundManager() = default;
    ~SoundManager() = default;

    MIX_Mixer* m_mixer = nullptr;
    MIX_Track* m_musicTrack = nullptr;
    std::map<std::string, MIX_Audio*> m_sounds;
    std::map<std::string, MIX_Audio*> m_musics;
};