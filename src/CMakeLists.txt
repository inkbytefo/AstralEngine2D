# CMakeList.txt: SDLDers için CMake projesi, kaynağı ekleyin ve tanımlayın
# projeye özgü mantık burada bulunur.
#

find_package(SDL3 REQUIRED)

# Kaynak dosyaları listesi
set(SOURCES
    "main.cpp"
    "common.h"
    "core/app.h"
    "core/app.cpp"
    "core/entity_manager.h"
    "core/entity_manager.cpp"
    "ecs/entity.h"
    "ecs/components.h"
    "math/vec2.h"
)

add_executable(Astral ${SOURCES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Astral PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(Astral PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Astral PRIVATE SDL3::SDL3)

# TODO: Gerekirse testleri ve yükleme hedeflerini ekleyin.
