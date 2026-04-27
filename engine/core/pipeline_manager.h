#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include "../math/vertex.h"
#include <map>
#include <SDL3/SDL.h>

namespace Astral {

// Pipeline cache key for avoiding duplicate pipelines
struct PipelineKey {
    std::string vertexShader;
    std::string fragmentShader;
    SDL_GPUTextureFormat renderTargetFormat;
    bool hasDepth;

    bool operator<(const PipelineKey& other) const {
        if (vertexShader != other.vertexShader) return vertexShader < other.vertexShader;
        if (fragmentShader != other.fragmentShader) return fragmentShader < other.fragmentShader;
        if (renderTargetFormat != other.renderTargetFormat) return renderTargetFormat < other.renderTargetFormat;
        return hasDepth < other.hasDepth;
    }
};

/**
 * @brief Concrete implementation of pipeline management
 */
class PipelineManager : public IPipelineManager {
private:
    SDL_GPUDevice* m_device{ nullptr };
    std::map<std::string, GpuPipeline> m_pipelines;

public:
    PipelineManager() = default;

    void setGPUDevice(SDL_GPUDevice* device) {
        m_device = device;
    }

    SDL_GPUGraphicsPipeline* createPipeline(const std::string& name,
                                           SDL_GPUShader* vertexShader,
                                           SDL_GPUShader* fragmentShader,
                                           SDL_GPUTextureFormat renderTargetFormat,
                                           bool enableDepth = true,
                                           SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_BACK) override {
        if (!m_device) return nullptr;

        SDL_GPUVertexAttribute attributes[5];
        Vertex::getAttributeDescriptions(attributes);

        SDL_GPUVertexBufferDescription vertexBufferDesc = Vertex::getBindingDescription();

        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = attributes;
        vertexInputState.num_vertex_attributes = 5;

        SDL_GPURasterizerState rasterizerState = {};
        rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
        rasterizerState.cull_mode = cullMode;
        rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        rasterizerState.enable_depth_clip = true;

        SDL_GPUMultisampleState multisampleState = {};
        multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;

        SDL_GPUDepthStencilState depthStencilState = {};
        depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        depthStencilState.enable_depth_test = enableDepth;
        depthStencilState.enable_depth_write = enableDepth;

        SDL_GPUColorTargetBlendState blendState = {};
        blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.color_write_mask = 0xF;
        blendState.enable_blend = true;

        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = renderTargetFormat;
        colorTargetDesc.blend_state = blendState;

        SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
        targetInfo.color_target_descriptions = &colorTargetDesc;
        targetInfo.num_color_targets = 1;
        targetInfo.depth_stencil_format = enableDepth ? SDL_GPU_TEXTUREFORMAT_D16_UNORM : SDL_GPU_TEXTUREFORMAT_INVALID;
        targetInfo.has_depth_stencil_target = enableDepth;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.vertex_shader = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;
        pipelineCreateInfo.vertex_input_state = vertexInputState;
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.rasterizer_state = rasterizerState;
        pipelineCreateInfo.multisample_state = multisampleState;
        pipelineCreateInfo.depth_stencil_state = depthStencilState;
        pipelineCreateInfo.target_info = targetInfo;

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipelineCreateInfo);

        if (pipeline) {
            m_pipelines[name] = GpuPipeline(pipeline, m_device);
            SDL_Log("PipelineManager: Pipeline '%s' created", name.c_str());
        }

        return pipeline;
    }

    SDL_GPUGraphicsPipeline* getPipeline(const std::string& name) const override {
        auto it = m_pipelines.find(name);
        return (it != m_pipelines.end()) ? it->second.get() : nullptr;
    }

    void cleanup() override {
        m_pipelines.clear();
    }
};

} // namespace Astral