#pragma once
#include "core/scene.h"
#include "core/asset_manager.h"
#include "core/gltf_loader.h"
#include "systems/render_system.h"

class SandboxScene : public Scene
{
public:
    SandboxScene();
    void init() override;
    void update(float deltaTime) override;
    void render(SDL_GPURenderPass* renderPass) override;
    void sDoAction(const std::string& actionName, bool started) override;
    void onMouseMove(float x, float y, float relX, float relY) override;
    void onMouseButton(int button, bool pressed, float x, float y) override;
    bool isRunning() const override { return m_running; }

private:
    void createCubeMesh();
    void loadShaders();
    void createMaterials();
    void loadGLTFModels();
    
    bool m_running = true;
    float m_rotation = 0.0f;
};
