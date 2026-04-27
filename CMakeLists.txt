# CMakeList.txt: Astral projesi için ana yapılandırma dosyası
cmake_minimum_required (VERSION 3.10)

# Destekleniyorsa MSVC derleyicileri için Çalışırken Yeniden Yükleme aracını etkinleştirin.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Astral")

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)


# Kaynak dosyaları listesi (Yeni hiyerarşiye göre)
set(SOURCES
    "games/sandbox/main.cpp"
    "games/sandbox/sandbox_scene.h"
    "games/sandbox/sandbox_scene.cpp"
    "common.h"
    "engine/core/app.h"
    "engine/core/app.cpp"
    "engine/core/entity_manager.h"
    "engine/core/entity_manager.cpp"
    "engine/core/gltf_loader.cpp"
    "engine/ecs/entity.h"
    "engine/ecs/components.h"
    "engine/math/vertex.h"
    "engine/renderer/sdl3_graphics_device.cpp"
    "engine/renderer/sdl3_renderer.cpp"
    "engine/editor/editor_manager.cpp"
    "engine/editor/panels/editor_panel.h"
    "engine/editor/panels/scene_hierarchy_panel.h"
    "engine/editor/panels/scene_hierarchy_panel.cpp"
    "engine/editor/panels/properties_panel.h"
    "engine/editor/panels/properties_panel.cpp"
    "engine/editor/panels/viewport_panel.h"
    "engine/editor/panels/viewport_panel.cpp"
    "engine/vendor/imgui/imgui.cpp"
    "engine/vendor/imgui/imgui_draw.cpp"
    "engine/vendor/imgui/imgui_tables.cpp"
    "engine/vendor/imgui/imgui_widgets.cpp"
    "engine/vendor/imgui/backends/imgui_impl_sdl3.cpp"
    "engine/vendor/imgui/backends/imgui_impl_sdlgpu3.cpp"
    "engine/vendor/imgui/ImGuizmo.cpp"
)

add_executable(Astral ${SOURCES} 
    "engine/core/scene.h" 
    "engine/core/scene.cpp"
    "engine/core/asset_manager.h"
    "engine/core/system_manager.h"
    "engine/core/system_manager.cpp"
    "engine/core/shader_loader.h"
    "engine/core/sound_manager.h" 
    "engine/core/level_loader.h"
    "engine/core/gltf_loader.h"
    "engine/core/json.hpp"
    "engine/systems/system.h"
    "engine/systems/input_system.h"
    "engine/systems/physics_system.h" 
    "engine/systems/render_system.h"
    "engine/systems/render_system.cpp" 
    "engine/systems/lifespan_system.h" 
    "engine/systems/text_system.h"
    "engine/systems/camera_system.h"
    "engine/systems/transform_system.h"
    "engine/renderer/graphics_device.h"
    "engine/renderer/renderer.h"
    "engine/renderer/sdl3_graphics_device.h"
    "engine/renderer/sdl3_renderer.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Astral PROPERTY CXX_STANDARD 20)
endif()

# Engine ve kök dizini include yollarına ekle
target_include_directories(Astral PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/vendor/imgui/backends
)

# Geliştirme aşamasında kaynak assets klasörüne erişmek için macro ve uyarıları kapatma
target_compile_definitions(Astral PRIVATE 
    ENGINE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(Astral PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer)

# ============================================================================
# OTOMATIK SHADER DERLEME SİSTEMİ (Vulkan SPIR-V)
# ============================================================================
# Vulkan SDK'nın glslc derleyicisini bul
find_program(GLSLC_EXECUTABLE glslc HINTS 
    "$ENV{VULKAN_SDK}/Bin" 
    "$ENV{VULKAN_SDK}/bin"
    "$ENV{VK_SDK_PATH}/Bin"
)

if(GLSLC_EXECUTABLE)
    message(STATUS "glslc bulundu: ${GLSLC_EXECUTABLE}")
    
    # Shader kaynak dosyaları (GLSL formatında) - Stage bilgisi ile
    set(VERTEX_SHADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic_3d.vert.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/pbr.vert.glsl"
    )
    
    set(FRAGMENT_SHADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic_3d.frag.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/pbr.frag.glsl"
    )
    
    # Vertex Shader'ları derle
    foreach(SHADER ${VERTEX_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=vertex ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Vertex Shader derleniyor: ${SHADER_NAME} -> ${SHADER_NAME}.spv"
            VERBATIM
        )
        
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()
    
    # Fragment Shader'ları derle
    foreach(SHADER ${FRAGMENT_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=fragment ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Fragment Shader derleniyor: ${SHADER_NAME} -> ${SHADER_NAME}.spv"
            VERBATIM
        )
        
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()
    
    # Shader derleme hedefi oluştur (Astral'a bağımlı)
    add_custom_target(CompileShaders ALL DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(Astral CompileShaders)
    
    message(STATUS "Otomatik shader derleme sistemi aktif!")
else()
    message(WARNING "glslc bulunamadı! Shader'lar otomatik derlenemeyecek.")
    message(WARNING "Vulkan SDK kurulu olduğundan emin olun: https://vulkan.lunarg.com/")
endif()

# Assets klasörünü build dizinine kopyala
add_custom_command(TARGET Astral POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:Astral>/assets"
)
