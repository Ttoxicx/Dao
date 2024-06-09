
set(ENGINE_ROOT_DIR "${DAO_ROOT_DIR}/engine")
set(THIRD_PARTY_DIR "${ENGINE_ROOT_DIR}/third_party")
set(ENGINE_ASSET_DIR "/asset")

set(VULKAN_DIR "${THIRD_PARTY_DIR}/vulkan")
set(VULKAN_INCLUDE_DIR "${VULKAN_DIR}/include")
set(VULKAN_LIB "vulkan-1.lib")

if(WIN32)
    set(VULKAN_BIN_DIR "${VULKAN_DIR}/bin/win")
    set(VULKAN_LIB_DIR "${VULKAN_DIR}/lib/win")
    set(glslangValidator_excutable "${VULKAN_BIN_DIR}/glslangValidator.exe")
    add_compile_definitions("DAO_VK_LAYER_PATH=${VULKAN_LIB_DIR}")
elseif(UNIX)
    message(FATAL_ERROR "Does not support UNIX")
else()
    message(FATAL_ERROR "Unknown Platform")
endif()

add_subdirectory(source/editor)
add_subdirectory(source/runtime)
add_subdirectory(source/meta_parser)
add_subdirectory(third_party)