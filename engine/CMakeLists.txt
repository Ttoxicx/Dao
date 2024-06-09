
set(ENGINE_ROOT_DIR "${DAO_ROOT_DIR}/engine")
set(THIRD_PARTY_DIR "${ENGINE_ROOT_DIR}/third_party")
set(ENGINE_ASSET_DIR "/asset")

set(VULKAN_INCLUDE_DIR "${THIRD_PARTY_DIR}/vulkan/include")

if(WIN32)
    set(VULKAN_LIB "${THIRD_PARTY_DIR}/vulkan/lib/win/vulkan-1.lib")
    set(glslangValidator_excutable "${THIRD_PARTY_DIR}/vulkan/bin/win/glslangValidator.exe")
    add_compile_definitions("DAO_VK_LAYER_PATH=${THIRD_PARTY_DIR}/vulkan/bin/win")
elseif(UNIX)
    message(FATAL_ERROR "Does not support UNIX")
else()
    message(FATAL_ERROR "Unknown Platform")
endif()

add_subdirectory(source/editor)
add_subdirectory(source/runtime)
add_subdirectory(source/meta_parser)
add_subdirectory(third_party)