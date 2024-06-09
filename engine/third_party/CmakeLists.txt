# glfw
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# assimp
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# spdlog
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)

# json11
set(JSON_INCLUDE_DIR "${THIRD_PARTY_DIR}/json11")
add_library(json11 "${JSON_INCLUDE_DIR}/json11.cpp")
target_include_directories(json11 PUBLIC ${JSON_INCLUDE_DIR})

# imgui
set(IMGUI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES "${IMGUI_SOURCE_DIR}/*.cpp")
file(GLOB IMGUI_IMPL 
"${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp" 
"${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.h"
"${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp" 
"${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.h")
add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_IMPL})
target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${IMGUI_SOURCE_DIR}>)
target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${VULKAN_INCLUDE_DIR}>)
target_link_libraries(imgui PUBLIC glfw ${VULKAN_LIB})

# stb
file(GLOB STB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/stb/*.h")
add_library(stb INTERFACE ${STB_SOURCES})
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)