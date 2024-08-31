# glfw
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

# spdlog
option(SPDLOG_BUILD_EXAMPLE "" OFF)
option(SPDLOG_INSTALL "" OFF)
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

# tiny_obj_loader
add_subdirectory(tinyobjloader)

# jolt
option(TARGET_HELLO_WORLD "" OFF)
option(TARGET_PERFORMANCE_TEST "" OFF)
option(TARGET_SAMPLES "" OFF)
option(TARGET_UNIT_TESTS "" OFF)
option(TARGET_VIEWER "" OFF)
option(TARGET_TEST_FRAMEWORK "" OFF)
add_subdirectory(JoltPhysics/Build)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-Wno-unqualified-std-cast-call" COMPILER_CHECK_UNQUALIFIED)
    if(COMPILER_CHECK_UNQUALIFIED)
        target_compile_options(Jolt PRIVATE "-Wno-unqualified-std-cast-call")
    endif()
endif()
set_target_properties(Jolt PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
