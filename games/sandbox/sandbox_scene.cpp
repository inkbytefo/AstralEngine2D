#include "sandbox_scene.h"
#include "core/app.h"
#include "core/shader_loader.h"
#include "math/vertex.h"
#include <glm/gtc/matrix_transform.hpp>

SandboxScene::SandboxScene()
{
}

void SandboxScene::init()
{
    // Aksiyonları kaydet
    registerAction(SDLK_ESCAPE, "QUIT");

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
    cameraEnt->add<CCamera>(
        glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),  // Kamera pozisyonu
            glm::vec3(0.0f, 0.0f, 0.0f),  // Bakış noktası
            glm::vec3(0.0f, 1.0f, 0.0f)   // Yukarı vektör
        ),
        glm::perspective(
            glm::radians(60.0f),  // FOV
            1280.0f / 720.0f,     // Aspect ratio
            0.1f,                 // Near plane
            100.0f                // Far plane
        )
    );
    cameraEnt->cCamera.isActive = true;

    // Küp entity'si oluştur (Parent)
    auto cubeEnt = m_entityManager.addEntity("cube");
    cubeEnt->add<CTransform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f));
    cubeEnt->cTransform.scale = glm::vec3(0.5f);
    cubeEnt->cTransform.rotation = glm::vec3(0.0f);
    cubeEnt->add<CMesh>("cube", "box_material"); // Material kullanıyoruz artık!

    // İkinci Küp entity'si oluştur (Child - Uydu/Ay)
    auto moonEnt = m_entityManager.addEntity("moon");
    moonEnt->add<CTransform>(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f)); // Parent'a göre 2 birim sağda
    moonEnt->cTransform.scale = glm::vec3(0.3f); // Parent'a göre %30 boyutta
    moonEnt->add<CMesh>("cube", "box_material");

    // Hiyerarşi bağlantısını kur (Scene Graph)
    moonEnt->cTransform.parent = cubeEnt;
    cubeEnt->cTransform.children.push_back(moonEnt);

    // ÖNEMLİ: Entity'leri aktif listeye taşı!
    m_entityManager.update();
    
    SDL_Log("SandboxScene initialized: %zu entities ready", m_entityManager.getEntities().size());
}

void SandboxScene::createCubeMesh()
{
    using namespace Astral;

    // 3D Küp Vertex'leri (Pozisyon, Renk, UV)
    std::vector<Vertex> vertices = {
        // Ön Yüz
        Vertex({-1, -1,  1}, {1, 0, 0}, {0, 1}), Vertex({ 1, -1,  1}, {0, 1, 0}, {1, 1}),
        Vertex({ 1,  1,  1}, {0, 0, 1}, {1, 0}), Vertex({-1,  1,  1}, {1, 1, 0}, {0, 0}),
        // Arka Yüz
        Vertex({-1, -1, -1}, {1, 0, 1}, {1, 1}), Vertex({-1,  1, -1}, {0, 1, 1}, {1, 0}),
        Vertex({ 1,  1, -1}, {1, 1, 1}, {0, 0}), Vertex({ 1, -1, -1}, {0, 0, 0}, {0, 1}),
        // Üst Yüz
        Vertex({-1,  1, -1}, {1, 0.5, 0}, {0, 0}), Vertex({-1,  1,  1}, {0.5, 1, 0}, {0, 1}),
        Vertex({ 1,  1,  1}, {0, 0.5, 1}, {1, 1}), Vertex({ 1,  1, -1}, {1, 0, 0.5}, {1, 0}),
        // Alt Yüz
        Vertex({-1, -1, -1}, {0, 1, 0.5}, {1, 0}), Vertex({ 1, -1, -1}, {0.5, 0, 1}, {0, 0}),
        Vertex({ 1, -1,  1}, {1, 0.5, 1}, {0, 1}), Vertex({-1, -1,  1}, {0.5, 1, 0.5}, {1, 1}),
        // Sağ Yüz
        Vertex({ 1, -1, -1}, {1, 0, 0}, {1, 1}), Vertex({ 1,  1, -1}, {0, 1, 0}, {1, 0}),
        Vertex({ 1,  1,  1}, {0, 0, 1}, {0, 0}), Vertex({ 1, -1,  1}, {1, 1, 0}, {0, 1}),
        // Sol Yüz
        Vertex({-1, -1, -1}, {1, 0, 1}, {0, 1}), Vertex({-1, -1,  1}, {0, 1, 1}, {1, 1}),
        Vertex({-1,  1,  1}, {1, 1, 1}, {1, 0}), Vertex({-1,  1, -1}, {0, 0, 0}, {0, 0})
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
        "assets/shaders/basic_3d.vert.glsl.spv",
        SDL_GPU_SHADERSTAGE_VERTEX,
        "main",
        0, // samplers
        0, // storage textures
        0, // storage buffers
        1  // uniform buffers (MVP matrisi için - Set 0, Binding 0)
    );

    SDL_GPUShader* fragShader = ShaderLoader::loadShaderFromFile(
        device,
        "assets/shaders/basic_3d.frag.glsl.spv",
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        "main",
        1, // samplers (albedoTex için - Set 2, Binding 0)
        0, // storage textures
        0, // storage buffers
        1  // uniform buffers (FragmentUniform için - Set 0, Binding 0)
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

    // Pipeline oluştur
    AssetManager::getInstance().createPipeline(
        "basic3d",
        vertShader,
        fragShader,
        swapchainFormat,
        true, // depth test enabled (3D Küp için)
        SDL_GPU_CULLMODE_BACK // backface culling enabled
    );

    SDL_Log("Pipeline 'basic3d' oluşturuldu!");
}

void SandboxScene::createMaterials()
{
    using namespace Astral;
    AssetManager& assetMgr = AssetManager::getInstance();

    // box.png yükle (assets/textures/ klasörüne atılmış olmalı)
    assetMgr.uploadTexture("box_tex", "assets/textures/box.png");

    // Material yarat (Pipeline + Texture'u birleştirir)
    assetMgr.createMaterial("box_material", "basic3d", "box_tex");

    SDL_Log("Material 'box_material' oluşturuldu!");
}

void SandboxScene::update(float deltaTime)
{
    // Üçgeni döndür
    m_rotation += 1.0f * deltaTime;
    
    for (auto& entity : m_entityManager.getEntities()) {
        if (entity->tag() == "cube" && entity->has<CTransform>()) {
            entity->get<CTransform>().rotation.x += 0.5f * deltaTime;
            entity->get<CTransform>().rotation.y += 1.0f * deltaTime;
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
}
