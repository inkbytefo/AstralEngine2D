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

# Kaynak dosyaları listesi (Yeni hiyerarşiye göre)
set(SOURCES
    "games/pong/main.cpp"
    "common.h"
    "engine/core/app.h"
    "engine/core/app.cpp"
    "engine/core/entity_manager.h"
    "engine/core/entity_manager.cpp"
    "engine/ecs/entity.h"
    "engine/ecs/components.h"
    "engine/math/vec2.h"
)

add_executable(Astral ${SOURCES} "engine/core/scene.h" "games/pong/pong_game.h" "games/pong/pong_game.cpp" "engine/systems/movement_system.h" "engine/systems/render_system.h" "engine/systems/lifespan_system.h" "engine/systems/text_system.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Astral PROPERTY CXX_STANDARD 20)
endif()

# Engine ve kök dizini include yollarına ekle
target_include_directories(Astral PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
)

target_link_libraries(Astral PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

# Assets klasörünü build dizinine kopyala
add_custom_command(TARGET Astral POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:Astral>/assets")
