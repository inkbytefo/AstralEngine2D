#pragma once
#include <string>
#include <vector>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// CGLTF_IMPLEMENTATION burada TANIMLANMAMALI! (Sadece gltf_loader.cpp'de olacak)
#include "../vendor/cgltf/cgltf.h"

#include "asset_manager.h"
#include "../ecs/entity.h"
#include "../ecs/components.h"
#include "../math/vertex.h"

namespace Astral {

// GLTF Node'dan Entity'ye dönüşüm için geçici veri yapısı
struct GLTFNodeData {
    std::shared_ptr<Astral::Entity> entity;
    size_t nodeIndex;
    size_t parentIndex;
    bool hasParent;
};

class GLTFLoader {
public:
    // GLTF dosyasını yükle ve Entity hiyerarşisi oluştur
    // rootEntity: Yüklenen modelin kök entity'si (nullptr ise yeni oluşturulur)
    // Returns: Root entity pointer
    static std::shared_ptr<Astral::Entity> loadGLTF(
        const std::string& filepath,
        Astral::EntityManager& entityManager,
        AssetManager& assetMgr,
        const std::string& materialPrefix = "gltf_mat_",
        std::shared_ptr<Astral::Entity> rootEntity = nullptr)
    {
        cgltf_options options = {};
        cgltf_data* data = nullptr;
        
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
        if (result != cgltf_result_success) {
            SDL_Log("GLTF Parse Hatası: %s", filepath.c_str());
            return nullptr;
        }

        result = cgltf_load_buffers(&options, data, filepath.c_str());
        if (result != cgltf_result_success) {
            SDL_Log("GLTF Buffer Yükleme Hatası: %s", filepath.c_str());
            cgltf_free(data);
            return nullptr;
        }

        SDL_Log("GLTF Yükleniyor: %s", filepath.c_str());
        SDL_Log("  Meshler: %zu", data->meshes_count);
        SDL_Log("  Materyaller: %zu", data->materials_count);
        SDL_Log("  Node'lar: %zu", data->nodes_count);

        // Dosya dizinini al (dokuları bulmak için)
        std::string baseDir = "";
        size_t lastSlash = filepath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            baseDir = filepath.substr(0, lastSlash + 1);
        }

        // 1. Materyalleri yükle (BaseDir ile birlikte)
        loadMaterials(data, assetMgr, materialPrefix, baseDir);

        // 2. Mesh'leri yükle (Her primitive ayrı mesh olarak)
        loadMeshes(data, assetMgr, filepath);

        // 3. Scene Graph'i oluştur
        if (!rootEntity) {
            rootEntity = entityManager.addEntity("gltf_root");
        }

        std::vector<GLTFNodeData> nodeDataList;
        loadNodes(data, entityManager, nodeDataList, rootEntity, materialPrefix);

        cgltf_free(data);
        
        SDL_Log("GLTF Yükleme Tamamlandı: %s", filepath.c_str());
        return rootEntity;
    }

private:
    // Materyalleri AssetManager'a yükle
    static void loadMaterials(cgltf_data* data, AssetManager& assetMgr, const std::string& prefix, const std::string& baseDir) {
        for (size_t i = 0; i < data->materials_count; ++i) {
            cgltf_material* mat = &data->materials[i];
            std::string matName = prefix + (mat->name ? mat->name : ("mat_" + std::to_string(i)));

            std::string albedoTexName = "";
            std::string normalTexName = "";
            std::string mrTexName = "";

            // 1. Albedo Texture
            if (mat->has_pbr_metallic_roughness && mat->pbr_metallic_roughness.base_color_texture.texture) {
                cgltf_image* img = mat->pbr_metallic_roughness.base_color_texture.texture->image;
                if (img && img->uri) {
                    std::string texPath = baseDir + img->uri;
                    albedoTexName = matName + "_albedo";
                    assetMgr.uploadTexture(albedoTexName, texPath);
                }
            }

            // 2. Normal Texture
            if (mat->normal_texture.texture) {
                cgltf_image* img = mat->normal_texture.texture->image;
                if (img && img->uri) {
                    std::string texPath = baseDir + img->uri;
                    normalTexName = matName + "_normal";
                    assetMgr.uploadTexture(normalTexName, texPath);
                }
            }

            // 3. Metallic-Roughness Texture
            if (mat->has_pbr_metallic_roughness && mat->pbr_metallic_roughness.metallic_roughness_texture.texture) {
                cgltf_image* img = mat->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
                if (img && img->uri) {
                    std::string texPath = baseDir + img->uri;
                    mrTexName = matName + "_mr";
                    assetMgr.uploadTexture(mrTexName, texPath);
                }
            }

            // PBR Değerleri
            glm::vec4 baseColor(1.0f);
            float metallic = 1.0f;
            float roughness = 1.0f;

            if (mat->has_pbr_metallic_roughness) {
                auto& pbr = mat->pbr_metallic_roughness;
                baseColor = glm::vec4(
                    pbr.base_color_factor[0],
                    pbr.base_color_factor[1],
                    pbr.base_color_factor[2],
                    pbr.base_color_factor[3]
                );
                metallic = pbr.metallic_factor;
                roughness = pbr.roughness_factor;
            }

            // Material oluştur
            assetMgr.createMaterial(matName, "pbr_pipeline", albedoTexName, normalTexName, mrTexName, baseColor, metallic, roughness);
            
            SDL_Log("  Material: %s (Textures Loaded)", matName.c_str());
        }
    }

    // Mesh'leri Mega-Buffer'a yükle
    static void loadMeshes(cgltf_data* data, AssetManager& assetMgr, const std::string& filepath) {
        for (size_t meshIdx = 0; meshIdx < data->meshes_count; ++meshIdx) {
            cgltf_mesh* mesh = &data->meshes[meshIdx];
            
            for (size_t primIdx = 0; primIdx < mesh->primitives_count; ++primIdx) {
                cgltf_primitive* prim = &mesh->primitives[primIdx];
                
                // Sadece triangle list destekliyoruz
                if (prim->type != cgltf_primitive_type_triangles) continue;

                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                // Vertex verilerini oku
                extractVertices(prim, vertices);
                extractIndices(prim, indices);

                if (vertices.empty() || indices.empty()) continue;

                // Mesh ismini oluştur
                std::string meshName = "gltf_";
                if (mesh->name) meshName += std::string(mesh->name) + "_";
                meshName += "prim_" + std::to_string(primIdx);

                // Mega-Buffer'a yükle
                assetMgr.uploadMesh(meshName, vertices, indices);
                
                SDL_Log("  Mesh: %s (V:%zu I:%zu)", meshName.c_str(), vertices.size(), indices.size());
            }
        }
    }

    // Vertex verilerini çıkar
    static void extractVertices(cgltf_primitive* prim, std::vector<Vertex>& vertices) {
        cgltf_accessor* posAccessor = nullptr;
        cgltf_accessor* normalAccessor = nullptr;
        cgltf_accessor* texCoordAccessor = nullptr;
        cgltf_accessor* tangentAccessor = nullptr;

        // Attribute'ları bul
        for (size_t i = 0; i < prim->attributes_count; ++i) {
            cgltf_attribute* attr = &prim->attributes[i];
            if (attr->type == cgltf_attribute_type_position) posAccessor = attr->data;
            else if (attr->type == cgltf_attribute_type_normal) normalAccessor = attr->data;
            else if (attr->type == cgltf_attribute_type_texcoord) texCoordAccessor = attr->data;
            else if (attr->type == cgltf_attribute_type_tangent) tangentAccessor = attr->data;
        }

        if (!posAccessor) return;

        size_t vertexCount = posAccessor->count;
        vertices.resize(vertexCount);

        // Position (X, Y, Z) -> (X, -Z, Y) Dönüşümü
        for (size_t i = 0; i < vertexCount; ++i) {
            float pos[3];
            cgltf_accessor_read_float(posAccessor, i, pos, 3);
            vertices[i].pos = glm::vec3(pos[0], -pos[2], pos[1]);
        }

        // Normal (X, Y, Z) -> (X, -Z, Y) Dönüşümü
        if (normalAccessor) {
            for (size_t i = 0; i < vertexCount; ++i) {
                float norm[3];
                cgltf_accessor_read_float(normalAccessor, i, norm, 3);
                vertices[i].normal = glm::vec3(norm[0], -norm[2], norm[1]);
            }
        } else {
            // Default normal
            for (size_t i = 0; i < vertexCount; ++i) {
                vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        // TexCoord
        if (texCoordAccessor) {
            for (size_t i = 0; i < vertexCount; ++i) {
                float uv[2];
                cgltf_accessor_read_float(texCoordAccessor, i, uv, 2);
                vertices[i].uv = glm::vec2(uv[0], uv[1]);
            }
        } else {
            // Default UV
            for (size_t i = 0; i < vertexCount; ++i) {
                vertices[i].uv = glm::vec2(0.0f, 0.0f);
            }
        }

        // Tangent (X, Y, Z) -> (X, -Z, Y) Dönüşümü
        if (tangentAccessor) {
            for (size_t i = 0; i < vertexCount; ++i) {
                float tangent[4];
                cgltf_accessor_read_float(tangentAccessor, i, tangent, 4);
                vertices[i].tangent = glm::vec4(tangent[0], -tangent[2], tangent[1], tangent[3]);
            }
        } else {
            // Default tangent
            for (size_t i = 0; i < vertexCount; ++i) {
                vertices[i].tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // Color (GLTF'de genelde yok, default beyaz)
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // Index verilerini çıkar
    static void extractIndices(cgltf_primitive* prim, std::vector<uint32_t>& indices) {
        if (!prim->indices) return;

        cgltf_accessor* accessor = prim->indices;
        indices.resize(accessor->count);

        for (size_t i = 0; i < accessor->count; ++i) {
            indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(accessor, i));
        }
    }

    // Node hiyerarşisini oluştur
    static void loadNodes(
        cgltf_data* data,
        Astral::EntityManager& entityManager,
        std::vector<GLTFNodeData>& nodeDataList,
        std::shared_ptr<Astral::Entity> rootEntity,
        const std::string& materialPrefix)
    {
        // Tüm node'ları entity'lere dönüştür
        for (size_t i = 0; i < data->nodes_count; ++i) {
            cgltf_node* node = &data->nodes[i];
            
            std::string entityName = node->name ? node->name : ("node_" + std::to_string(i));
            auto entity = entityManager.addEntity(entityName);

            // Transform'u ayarla
            auto& transform = entity->add<CTransform>();
            
            if (node->has_matrix) {
                // Matrix (Y-Up -> Z-Up dönüşümü)
                glm::mat4 mat;
                memcpy(glm::value_ptr(mat), node->matrix, sizeof(float) * 16);
                
                // Koordinat sistemi dönüşüm matrisi
                glm::mat4 conv = glm::mat4(1.0f);
                conv[1][1] = 0; conv[1][2] = 1;
                conv[2][1] = -1; conv[2][2] = 0;
                mat = conv * mat;

                // Matrix'ten TRS çıkar
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat rotation;
                glm::decompose(mat, scale, rotation, translation, skew, perspective);
                
                transform.pos = translation;
                transform.scale = scale;
                transform.rotation = glm::eulerAngles(rotation);
            } else {
                // TRS ayrı ayrı (Y-Up -> Z-Up dönüşümü)
                if (node->has_translation) {
                    transform.pos = glm::vec3(node->translation[0], -node->translation[2], node->translation[1]);
                }
                if (node->has_rotation) {
                    // Quaternion dönüşümü biraz daha karmaşık ama basitleştirilmiş swap
                    glm::quat q(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
                    // Y-Up -> Z-Up için quat rotasyonu
                    glm::quat rotFix = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
                    transform.rotation = glm::eulerAngles(rotFix * q);
                }
                if (node->has_scale) {
                    transform.scale = glm::vec3(node->scale[0], node->scale[2], node->scale[1]);
                }
            }

            // Mesh varsa her primitive için AYRI entity oluştur
            if (node->mesh) {
                cgltf_mesh* mesh = node->mesh;
                
                // İlk primitive'i bu entity'ye ekle
                if (mesh->primitives_count > 0) {
                    cgltf_primitive* prim = &mesh->primitives[0];
                    
                    std::string meshName = "gltf_";
                    if (mesh->name) meshName += std::string(mesh->name) + "_";
                    meshName += "prim_0";

                    std::string matName = "default";
                    if (prim->material && prim->material->name) {
                        matName = materialPrefix + std::string(prim->material->name);
                    }

                    entity->add<CMesh>(meshName, matName);
                }
                
                // Diğer primitive'ler için child entity'ler oluştur
                for (size_t primIdx = 1; primIdx < mesh->primitives_count; ++primIdx) {
                    cgltf_primitive* prim = &mesh->primitives[primIdx];
                    
                    std::string meshName = "gltf_";
                    if (mesh->name) meshName += std::string(mesh->name) + "_";
                    meshName += "prim_" + std::to_string(primIdx);

                    std::string matName = "default";
                    if (prim->material && prim->material->name) {
                        matName = materialPrefix + std::string(prim->material->name);
                    }

                    // Child entity oluştur
                    auto childEntity = entityManager.addEntity(entityName + "_prim_" + std::to_string(primIdx));
                    auto& childTransform = childEntity->add<CTransform>();
                    childTransform.parent = entity;
                    childEntity->add<CMesh>(meshName, matName);
                    
                    transform.children.push_back(childEntity);
                }
            }

            nodeDataList.push_back({ entity, i, 0, false });
        }

        // Parent-child ilişkilerini kur
        for (size_t i = 0; i < data->nodes_count; ++i) {
            cgltf_node* node = &data->nodes[i];
            
            for (size_t childIdx = 0; childIdx < node->children_count; ++childIdx) {
                cgltf_node* childNode = node->children[childIdx];
                size_t childIndex = childNode - data->nodes;
                
                auto& parentTransform = nodeDataList[i].entity->get<CTransform>();
                auto& childTransform = nodeDataList[childIndex].entity->get<CTransform>();
                
                childTransform.parent = nodeDataList[i].entity;
                parentTransform.children.push_back(nodeDataList[childIndex].entity);
            }
        }

        // Root node'ları ana entity'ye bağla
        if (data->scene && data->scene->nodes_count > 0) {
            auto& rootTransform = rootEntity->add<CTransform>();
            
            for (size_t i = 0; i < data->scene->nodes_count; ++i) {
                cgltf_node* sceneNode = data->scene->nodes[i];
                size_t nodeIndex = sceneNode - data->nodes;
                
                auto& childTransform = nodeDataList[nodeIndex].entity->get<CTransform>();
                childTransform.parent = rootEntity;
                rootTransform.children.push_back(nodeDataList[nodeIndex].entity);
            }
        }
    }
};

} // namespace Astral
