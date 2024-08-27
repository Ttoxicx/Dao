file(GLOB_RECURSE DAO_EDITOR_SRC 
    "*.h"
    "*.cpp"
)

add_executable(DaoEditor ${DAO_EDITOR_SRC})

target_link_libraries(DaoEditor PRIVATE DaoRuntime)

add_custom_command(
    TARGET DaoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSET_DIR} $<TARGET_FILE_DIR:DaoEditor>/asset
)
add_custom_command(
    TARGET DaoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${RESOURCE_DIR} $<TARGET_FILE_DIR:DaoEditor>/resource
)
add_custom_command(
    TARGET DaoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CONFIG_DIR} $<TARGET_FILE_DIR:DaoEditor>/config
)

install(TARGETS DaoEditor RUNTIME DESTINATION .)
