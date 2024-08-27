
set(ENGINE_ROOT_DIR "${DAO_ROOT_DIR}/engine")
set(THIRD_PARTY_DIR "${ENGINE_ROOT_DIR}/third_party")
set(ENGINE_ASSET_DIR "/asset")

set(VULKAN_INCLUDE_DIR "${THIRD_PARTY_DIR}/vulkan/include")

if(WIN32)
    set(VULKAN_LIB "${THIRD_PARTY_DIR}/vulkan/lib/win/vulkan-1.lib")
    set(glslangValidator_executable "${THIRD_PARTY_DIR}/vulkan/bin/win/glslangValidator.exe")
    add_compile_definitions("DAO_VK_LAYER_PATH=${THIRD_PARTY_DIR}/vulkan/bin/win")
elseif(UNIX)
    message(FATAL_ERROR "Does not support UNIX")
else()
    message(FATAL_ERROR "Unknown Platform")
endif()

set(ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/asset)
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resource)
set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config)

install(DIRECTORY "${ASSET_DIR}/" DESTINATION "asset" FILES_MATCHING PATTERN "*")
install(DIRECTORY "${RESOURCE_DIR}/" DESTINATION "resource" FILES_MATCHING PATTERN "*")
install(DIRECTORY "${CONFIG_DIR}/" DESTINATION "config" FILES_MATCHING PATTERN "*")

set(SHADER_COMPILE_TARGET DaoShaderCompile)
add_subdirectory(shader)

add_subdirectory(third_party)

add_subdirectory(source/editor)
add_subdirectory(source/runtime)
add_subdirectory(source/meta_parser)

set(CODEGEN_TARGET "DaoPreCompile")
include(source/precompile/precompile.cmake)
add_dependencies(DaoRuntime "${CODEGEN_TARGET}")
add_dependencies("${CODEGEN_TARGET}" "DaoParser")
# TODO(find way to prohibit install tinyobjloader)
install(
    CODE [[
       file(REMOVE_RECURSE 
           ${CMAKE_INSTALL_PREFIX}/include 
           ${CMAKE_INSTALL_PREFIX}/lib
           ${CMAKE_INSTALL_PREFIX}/share
       )
    ]]
)