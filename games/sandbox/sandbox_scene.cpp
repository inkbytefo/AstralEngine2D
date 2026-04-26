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

    // Üçgen mesh'i oluştur
    createTriangleMesh();

    // Shader'ları yükle ve pipeline oluştur
    loadShaders();

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

    // Üçgen entity'si oluştur
    auto triangleEnt = m_entityManager.addEntity("triangle");
    triangleEnt->add<CTransform>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f));
    triangleEnt->cTransform.scale = glm::vec3(1.0f);
    triangleEnt->cTransform.rotation = glm::vec3(0.0f);
    
    triangleEnt->add<CMesh>("triangle", "basic3d");

    // ÖNEMLİ: Entity'leri aktif listeye taşı!
    m_entityManager.update();
    
    SDL_Log("SandboxScene initialized: %zu entities ready", m_entityManager.getEntities().size());
}

void SandboxScene::createTriangleMesh()
{
    using namespace Astral;

    // Renkli üçgen vertex'leri (32-byte optimal layout)
    std::vector<Vertex> vertices = {
        // Position                  Color (RGB)              UV
        Vertex(glm::vec3( 0.0f,  0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.5f, 0.0f)), // Üst (Kırmızı)
        Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)), // Sol alt (Yeşil)
        Vertex(glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f))  // Sağ alt (Mavi)
    };

    // Index buffer (saat yönünün tersine - counter-clockwise)
    std::vector<uint32_t> indices = { 0, 1, 2 };

    // Mesh'i GPU'ya yükle
    AssetManager::getInstance().uploadMesh("triangle", vertices, indices);
}

void SandboxScene::loadShaders()
{
    using namespace Astral;
    
    SDL_GPUDevice* device = m_app->getGPUDevice();
    
    // Shader dosyalarını yükle (SPIRV formatında)
    SDL_GPUShader* vertShader = ShaderLoader::loadShaderFromFile(
        device,
        "assets/shaders/basic_3d.vert.spv",
        SDL_GPU_SHADERSTAGE_VERTEX,
        "main",
        0, // samplers
        0, // storage textures
        0, // storage buffers
        1  // uniform buffers (MVP matrisleri için)
    );

    SDL_GPUShader* fragShader = ShaderLoader::loadShaderFromFile(
        device,
        "assets/shaders/basic_3d.frag.spv",
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        "main",
        0, // samplers
        0, // storage textures
        0, // storage buffers
        0  // uniform buffers
    );

    if (!vertShader || !fragShader) {
        SDL_Log("HATA: Shader'lar yüklenemedi!");
        SDL_Log("Lütfen assets/shaders/ klasöründe .spv dosyalarını derleyin!");
        return;
    }

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
        false, // depth test disabled (2D üçgen için)
        SDL_GPU_CULLMODE_NONE // culling disabled
    );

    SDL_Log("Pipeline 'basic3d' oluşturuldu!");
}

void SandboxScene::update(float deltaTime)
{
    // Üçgeni döndür
    m_rotation += 1.0f * deltaTime;
    
    for (auto& entity : m_entityManager.getEntities()) {
        if (entity->tag() == "triangle" && entity->has<CTransform>()) {
            entity->get<CTransform>().rotation.z = m_rotation;
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
