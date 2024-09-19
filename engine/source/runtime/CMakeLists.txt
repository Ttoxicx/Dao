
# PackageProject.cmake will be used to make our target installable
set(TARGET_NAME DaoRuntime)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

#source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${TARGET_NAME} INTERFACE)
add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "engine")

# Link dependencies    
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog)
target_link_libraries(${TARGET_NAME} PUBLIC imgui)
target_link_libraries(${TARGET_NAME} PUBLIC ${VULKAN_LIB})
target_link_libraries(${TARGET_NAME} PUBLIC json11)
target_link_libraries(${TARGET_NAME} PUBLIC Jolt)
target_link_libraries(${TARGET_NAME} PUBLIC stb)
target_link_libraries(${TARGET_NAME} PUBLIC lua sol2)
target_link_libraries(${TARGET_NAME} PRIVATE tinyobjloader)

add_dependencies(${TARGET_NAME} ${SHADER_COMPILE_TARGET})

target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_ROOT_DIR}/shader/_generated/cpp)

target_include_directories(${TARGET_NAME} PUBLIC ${VULKAN_INCLUDE_DIR})

target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_ROOT_DIR}/source)

target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_ROOT_DIR}/source/runtime)

# set global vari used by precompile
set(DAO_RUNTIME_HEADS "${HEADER_FILES}" PARENT_SCOPE)