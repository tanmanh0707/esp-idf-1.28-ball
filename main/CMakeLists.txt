# Define source files
set(SRC_DIRS_LIST
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/button"
    "${CMAKE_CURRENT_LIST_DIR}/configs"
    "${CMAKE_CURRENT_LIST_DIR}/log_app"
    "${CMAKE_CURRENT_LIST_DIR}/preferences"
    "${CMAKE_CURRENT_LIST_DIR}/system"
    "${CMAKE_CURRENT_LIST_DIR}/utils"
    "${CMAKE_CURRENT_LIST_DIR}/display"
    "${CMAKE_CURRENT_LIST_DIR}/AnimatedGIF"
    "${CMAKE_CURRENT_LIST_DIR}/gifplayer"
    "${CMAKE_CURRENT_LIST_DIR}/sdcard"
    "${CMAKE_CURRENT_LIST_DIR}/lib/LovyanGFX/src/lgfx/v1"
    "${CMAKE_CURRENT_LIST_DIR}/lib/LovyanGFX/src/lgfx/v1/misc"
    "${CMAKE_CURRENT_LIST_DIR}/lib/LovyanGFX/src/lgfx/v1/panel"
    "${CMAKE_CURRENT_LIST_DIR}/lib/LovyanGFX/src/lgfx/v1/platforms/esp32"
)

set(INCLUDE_DIRS_LIST
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/button"
    "${CMAKE_CURRENT_LIST_DIR}/configs"
    "${CMAKE_CURRENT_LIST_DIR}/log_app"
    "${CMAKE_CURRENT_LIST_DIR}/preferences"
    "${CMAKE_CURRENT_LIST_DIR}/system"
    "${CMAKE_CURRENT_LIST_DIR}/utils"
    "${CMAKE_CURRENT_LIST_DIR}/display"
    "${CMAKE_CURRENT_LIST_DIR}/AnimatedGIF"
    "${CMAKE_CURRENT_LIST_DIR}/gifplayer"
    "${CMAKE_CURRENT_LIST_DIR}/sdcard"
    "${CMAKE_CURRENT_LIST_DIR}/lib/LovyanGFX/src"
    "${CMAKE_CURRENT_LIST_DIR}/lib/LovyanGFX/src/lgfx/v1"
)

if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET "esp32s3")
endif()

# idf_build_get_property(build_components BUILD_COMPONENTS)
# # Function to find component dynamically by pattern
# function(find_component_by_pattern PATTERN COMPONENT_VAR PATH_VAR)
#     foreach(COMPONENT ${build_components})
#         if(COMPONENT MATCHES "${PATTERN}")
#             set(${COMPONENT_VAR} ${COMPONENT} PARENT_SCOPE)
#             idf_component_get_property(COMPONENT_PATH ${COMPONENT} COMPONENT_DIR)
#             set(${PATH_VAR} "${COMPONENT_PATH}" PARENT_SCOPE)
#             break()
#         endif()
#     endforeach()
# endfunction()

idf_component_register(
                    INCLUDE_DIRS ${INCLUDE_DIRS_LIST}
                    SRC_DIRS ${SRC_DIRS_LIST}
                    WHOLE_ARCHIVE
)
