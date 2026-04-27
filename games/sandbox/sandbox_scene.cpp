#include "sandbox_scene.h"
#include "core/app.h"
#include "core/shader_loader.h"
#include "math/vertex.h"
#include "systems/render_system.h"
#include "systems/camera_system.h"
#include <glm/gtc/matrix_transform.hpp>

SandboxScene::SandboxScene()
{
}

void SandboxScene::init()
{
    // Aksiyonları kaydet
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_W, "UP");
    registerAction(SDLK_S, "DOWN");
    registerAction(SDLK_A, "LEFT");
    registerAction(SDLK_D, "RIGHT");

    // AssetManager'a GPU device'ı ver
    Astral::AssetManager& assetMgr = Astral::AssetManager::getInstance();
    assetMgr.setGPUDevice(m_app->getGPUDevice());

    // Cube mesh'i oluştur
    createCubeMesh();

    // Shader'ları yükle ve pipeline oluştur
    loadShaders();

    // Texture ve Material oluştur
    createMaterials();

    // Kamera entity'si oluştur
    auto cameraEnt = m_entityManager.addEntity("camera");
    cameraEnt->add<CTransform>(glm::vec3(10.0f, 0.0f, 2.0f), glm::vec3(0.0f)); // X+ ileride, Z+ yukarıda
    cameraEnt->add<CInput>();
    cameraEnt->add<CFreeLook>(60.0f, 0.1f); // Hız 60'a çıkarıldı (2x)
    auto& look = cameraEnt->get<CFreeLook>();
    look.yaw = 180.0f; // Merkeze bakması için (X- yönüne)
    look.pitch = -10.0f; 

    cameraEnt->add<CCamera>(
        glm::mat4(1.0f),
        glm::perspective(
            glm::radians(60.0f),  // FOV
            1280.0f / 720.0f,     // Aspect ratio
            0.1f,                 // Near plane
            1000.0f               // Far plane
        )
    );
    cameraEnt->cCamera.isActive = true;

    // Işık (Güneş) - Z-Up'a göre ayarlandı
    auto sunEnt = m_entityManager.addEntity("sun");
    sunEnt->add<CLight>(
        glm::vec3(1.0f, 0.95f, 0.8f), 
        3.0f,                         
        glm::normalize(glm::vec3(-1.0f, -0.5f, -1.0f)) // Z ekseni aşağı doğru (yere doğru)
    );

    // ÖNEMLİ: Entity'leri aktif listeye taşı!
    m_entityManager.update();
    
    // GLTF Modelleri yükle (update'ten SONRA!)
    loadGLTFModels();
    
    // GLTF entity'lerini de aktif listeye taşı
    m_entityManager.update();
    
    SDL_Log("SandboxScene initialized: %zu entities ready", m_entityManager.getEntities().size());
}

void SandboxScene::createCubeMesh()
{
    using namespace Astral;

    // 3D Küp Vertex'leri (Pos, Normal, UV, Tangent, Color)
    glm::vec4 c1(1,0,0,1), c2(0,1,0,1), c3(0,0,1,1), c4(1,1,0,1), c5(1,0,1,1), c6(0,1,1,1);
    
    std::vector<Vertex> vertices = {
        // Ön Yüz (Normal: 0,0,1 | Tangent: 1,0,0,1)
        Vertex({-1, -1,  1}, {0, 0, 1}, {0, 1}, {1, 0, 0, 1}, c1), Vertex({ 1, -1,  1}, {0, 0, 1}, {1, 1}, {1, 0, 0, 1}, c1),
        Vertex({ 1,  1,  1}, {0, 0, 1}, {1, 0}, {1, 0, 0, 1}, c1), Vertex({-1,  1,  1}, {0, 0, 1}, {0, 0}, {1, 0, 0, 1}, c1),
        // Arka Yüz (Normal: 0,0,-1 | Tangent: -1,0,0,1)
        Vertex({-1, -1, -1}, {0, 0, -1}, {1, 1}, {-1, 0, 0, 1}, c2), Vertex({-1,  1, -1}, {0, 0, -1}, {1, 0}, {-1, 0, 0, 1}, c2),
        Vertex({ 1,  1, -1}, {0, 0, -1}, {0, 0}, {-1, 0, 0, 1}, c2), Vertex({ 1, -1, -1}, {0, 0, -1}, {0, 1}, {-1, 0, 0, 1}, c2),
        // Üst Yüz (Normal: 0,1,0 | Tangent: 1,0,0,1)
        Vertex({-1,  1, -1}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}, c3), Vertex({-1,  1,  1}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}, c3),
        Vertex({ 1,  1,  1}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}, c3), Vertex({ 1,  1, -1}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}, c3),
        // Alt Yüz (Normal: 0,-1,0 | Tangent: 1,0,0,1)
        Vertex({-1, -1, -1}, {0, -1, 0}, {1, 0}, {1, 0, 0, 1}, c4), Vertex({ 1, -1, -1}, {0, -1, 0}, {0, 0}, {1, 0, 0, 1}, c4),
        Vertex({ 1, -1,  1}, {0, -1, 0}, {0, 1}, {1, 0, 0, 1}, c4), Vertex({-1, -1,  1}, {0, -1, 0}, {1, 1}, {1, 0, 0, 1}, c4),
        // Sağ Yüz (Normal: 1,0,0 | Tangent: 0,0,-1,1)
        Vertex({ 1, -1, -1}, {1, 0, 0}, {1, 1}, {0, 0, -1, 1}, c5), Vertex({ 1,  1, -1}, {1, 0, 0}, {1, 0}, {0, 0, -1, 1}, c5),
        Vertex({ 1,  1,  1}, {1, 0, 0}, {0, 0}, {0, 0, -1, 1}, c5), Vertex({ 1, -1,  1}, {1, 0, 0}, {0, 1}, {0, 0, -1, 1}, c5),
        // Sol Yüz (Normal: -1,0,0 | Tangent: 0,0,1,1)
        Vertex({-1, -1, -1}, {-1, 0, 0}, {0, 1}, {0, 0, 1, 1}, c6), Vertex({-1, -1,  1}, {-1, 0, 0}, {1, 1}, {0, 0, 1, 1}, c6),
        Vertex({-1,  1,  1}, {-1, 0, 0}, {1, 0}, {0, 0, 1, 1}, c6), Vertex({-1,  1, -1}, {-1, 0, 0}, {0, 0}, {0, 0, 1, 1}, c6)
    };

    std::vector<uint32_t> indices = {
        0,  1,  2,  2,  3,  0,  // Ön
        4,  5,  6,  6,  7,  4,  // Arka
        8,  9, 10, 10, 11,  8,  // Üst
        12, 13, 14, 14, 15, 12, // Alt
        16, 17, 18, 18, 19, 16, // Sağ
        20, 21, 22, 22, 23, 20  // Sol
    };

    AssetManager::getInstance().uploadMesh("cube", vertices, indices);
}

void SandboxScene::loadShaders()
{
    using namespace Astral;
    
    SDL_GPUDevice* device = m_app->getGPUDevice();
    
    // Shader dosyalarını yükle (SPIRV formatında)
    SDL_GPUShader* vertShader = ShaderLoader::loadShaderFromFile(
        device,
        "assets/shaders/pbr.vert.glsl.spv",
        SDL_GPU_SHADERSTAGE_VERTEX,
        "main",
        0, // samplers
        0, // storage textures
        0, // storage buffers
        1  // uniform buffers (MVP matrisi için)
    );

    SDL_GPUShader* fragShader = ShaderLoader::loadShaderFromFile(
        device,
        "assets/shaders/pbr.frag.glsl.spv",
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        "main",
        3, // samplers (albedo, normal, metallicRoughness)
        0, // storage textures
        0, // storage buffers
        1  // uniform buffers (PBR Uniforms)
    );

    if (!vertShader || !fragShader) {
        SDL_Log("HATA: Shader'lar yüklenemedi!");
        SDL_Log("Lütfen assets/shaders/ klasöründe .spv dosyalarını derleyin!");
        return;
    }

    // Shader'ları AssetManager'a ekle (cleanup için)
    AssetManager::getInstance().addShader(vertShader);
    AssetManager::getInstance().addShader(fragShader);

    // Swapchain formatını al
    SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(
        device,
        m_app->getWindow()
    );

    // PBR Pipeline oluştur
    AssetManager::getInstance().createPipeline(
        "pbr_pipeline",
        vertShader,
        fragShader,
        swapchainFormat,
        true, // depth test enabled (3D için)
        SDL_GPU_CULLMODE_BACK // backface culling enabled
    );

    SDL_Log("Pipeline 'pbr_pipeline' oluşturuldu!");
}

void SandboxScene::createMaterials()
{
    using namespace Astral;
    AssetManager& assetMgr = AssetManager::getInstance();

    // box.png yükle (assets/textures/ klasörüne atılmış olmalı)
    assetMgr.uploadTexture("box_tex", "assets/textures/box.png");

    // PBR Material yarat (Albedo texture var, Normal/MR texture yok. Metallic: 0.1, Roughness: 0.8 ahşap kutu için uygun)
    assetMgr.createMaterial("box_material", "pbr_pipeline", "box_tex", "", "", glm::vec4(1.0f), 0.1f, 0.8f);

    SDL_Log("PBR Material 'box_material' oluşturuldu!");
}

void SandboxScene::loadGLTFModels()
{
    using namespace Astral;
    
    // Pipeline'ın hazır olduğundan emin ol
    if (!AssetManager::getInstance().getPipeline("pbr_pipeline")) {
        SDL_Log("UYARI: PBR Pipeline henüz hazır değil, GLTF yükleme atlanıyor");
        return;
    }

    // Default material oluştur (GLTF loader bunu kullanacak)
    AssetManager::getInstance().createMaterial("default", "pbr_pipeline", "", "", "", 
        glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.0f, 0.5f);

    // Hover Bike modelini yükle
    SDL_Log("=== GLTF Model Yükleniyor ===");
    auto bikeRoot = GLTFLoader::loadGLTF(
        "assets/test_models/hover_bike_-_the_rocket/scene.gltf", 
        m_entityManager,
        "bike_mat_"  // Material prefix
    );
    
    if (bikeRoot) {
        // Modeli sahneye yerleştir (Eksen dönüşümü artık loader içinde yapılıyor)
        auto& transform = bikeRoot->get<CTransform>();
        transform.pos = glm::vec3(0.0f, 0.0f, 0.0f);   
        transform.scale = glm::vec3(1.0f);              
        
        SDL_Log("Hover Bike modeli başarıyla yüklendi!");
        SDL_Log("Root entity tag: %s", bikeRoot->tag().c_str());
        
        // Yeni eklenen entity'leri toAdd listesinden ana listeye taşı ki sayabilelim
        m_entityManager.update();

        // Debug: Kaç tane mesh entity'si var?
        int meshCount = 0;
        for (auto& e : m_entityManager.getEntities()) {
            if (e->has<CMesh>()) {
                meshCount++;
                SDL_Log("  Mesh Entity: %s -> Material: %s", 
                    e->tag().c_str(), 
                    e->get<CMesh>().materialName.c_str());
            }
        }
        SDL_Log("Toplam Mesh Entity: %d", meshCount);
    } else {
        SDL_Log("HATA: Hover Bike modeli yüklenemedi!");
    }
    
    SDL_Log("=== GLTF Yükleme Tamamlandı ===");
}

void SandboxScene::update(float deltaTime)
{
    // Kamera Sistemini Güncelle
    Astral::CameraSystem::update(m_entityManager, deltaTime);
}

void SandboxScene::onMouseMove(float x, float y, float relX, float relY)
{
    for (auto& entity : m_entityManager.getEntities()) {
        if (entity->tag() == "camera" && entity->has<CFreeLook>()) {
            auto& look = entity->get<CFreeLook>();
            
            if (look.isRightMouseDown) {
                look.yaw += relX * look.sensitivity;
                look.pitch += relY * look.sensitivity; // Yukarı/Aşağı tersliği düzeltildi

                // Pitch limitleri (Unreal/Unity tarzı)
                if (look.pitch > 89.0f)  look.pitch = 89.0f;
                if (look.pitch < -89.0f) look.pitch = -89.0f;
            }
        }
    }
}

void SandboxScene::onMouseButton(int button, bool pressed, float x, float y)
{
    if (button == SDL_BUTTON_RIGHT) {
        for (auto& entity : m_entityManager.getEntities()) {
            if (entity->tag() == "camera" && entity->has<CFreeLook>()) {
                entity->get<CFreeLook>().isRightMouseDown = pressed;
                
                // Mouse'u kilitle/serbest bırak (Unreal Editor tarzı)
                SDL_SetWindowRelativeMouseMode(m_app->getWindow(), pressed);
            }
        }
    }
}

void SandboxScene::render(SDL_GPURenderPass* renderPass)
{
    // RenderSystem'e render pass'i set et (geçici çözüm)
    // NOT: Normalde RenderSystem kendi render pass'ini yönetir,
    // ama App.cpp'den gelen render pass'i kullanmak için bu gerekli
    
    // Şimdilik boş - RenderSystem'i App.cpp'den çağıracağız
}

void SandboxScene::sDoAction(const std::string& actionName, bool started)
{
    if (actionName == "QUIT" && started) {
        m_running = false;
    }

    // Kamera kontrolleri
    for (auto& entity : m_entityManager.getEntities()) {
        if (entity->tag() == "camera" && entity->has<CInput>()) {
            auto& input = entity->get<CInput>();
            if (actionName == "UP")    input.up = started;
            if (actionName == "DOWN")  input.down = started;
            if (actionName == "LEFT")  input.left = started;
            if (actionName == "RIGHT") input.right = started;
        }
    }
}
