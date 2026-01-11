cmake_minimum_required(VERSION 3.20)
project(cs_like LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_library(engine_lib STATIC
  src/engine/Engine.cpp
  src/platform/WindowSDL.cpp
  src/renderer/VulkanContext.cpp
  src/renderer/Swapchain.cpp
  src/renderer/Renderer.cpp
  src/core/Time.cpp
  src/core/Input.cpp
  # math можно не добавлять (они header-only), либо вынести в отдельный lib
)

target_include_directories(engine_lib PUBLIC src)
target_link_libraries(engine_lib PUBLIC SDL2::SDL2 Vulkan::Vulkan)

if (WIN32)
  target_link_libraries(engine_lib PUBLIC SDL2::SDL2main)
endif()

add_executable(cs_like
  src/main.cpp
  src/game/Player.cpp
  src/game/CameraFPS.cpp
)
target_link_libraries(cs_like PRIVATE engine_lib)


# ---- shaders (optional glslc build) ----
find_program(GLSLC glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin)

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/shaders_compiled)

file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

set(VERT_SRC ${SHADER_SRC_DIR}/triangle.vert)
set(FRAG_SRC ${SHADER_SRC_DIR}/triangle.frag)

set(VERT_SPV ${SHADER_BIN_DIR}/triangle.vert.spv)
set(FRAG_SPV ${SHADER_BIN_DIR}/triangle.frag.spv)

if (GLSLC)
  add_custom_command(
    OUTPUT ${VERT_SPV}
    COMMAND ${GLSLC} -o ${VERT_SPV} ${VERT_SRC}
    DEPENDS ${VERT_SRC}
    VERBATIM
  )
  add_custom_command(
    OUTPUT ${FRAG_SPV}
    COMMAND ${GLSLC} -o ${FRAG_SPV} ${FRAG_SRC}
    DEPENDS ${FRAG_SRC}
    VERBATIM
  )

  add_custom_target(shaders ALL DEPENDS ${VERT_SPV} ${FRAG_SPV})
  add_dependencies(cs_like shaders)
else()
  # fallback: assume precompiled spv exists in assets/shaders
  set(VERT_SPV ${SHADER_SRC_DIR}/triangle.vert.spv)
  set(FRAG_SPV ${SHADER_SRC_DIR}/triangle.frag.spv)
endif()

add_custom_command(TARGET cs_like POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cs_like>/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${VERT_SPV}
    $<TARGET_FILE_DIR:cs_like>/shaders/triangle.vert.spv
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${FRAG_SPV}
    $<TARGET_FILE_DIR:cs_like>/shaders/triangle.frag.spv
)

# Copy SDL2 runtime DLL next to exe (Windows)
if (WIN32)
  add_custom_command(TARGET cs_like POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2::SDL2>
      $<TARGET_FILE_DIR:cs_like>
  )
endif()
