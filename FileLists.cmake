SET(WATER_SURFACE_HEADERS
    ${WATER_SURFACE_SRC_DIR}/CpuWaterSim.h
    ${WATER_SURFACE_SRC_DIR}/WaterCharacter.h
    ${WATER_SURFACE_SRC_DIR}/WaterPlay.h)
    
SET(WATER_SURFACE_SOURCES
    ${WATER_SURFACE_SRC_DIR}/CpuWaterSim.cpp
    ${WATER_SURFACE_SRC_DIR}/WaterCharacter.cpp
    ${WATER_SURFACE_SRC_DIR}/WaterPlay.cpp
    ${WATER_SURFACE_SRC_DIR}/main.cpp)

SET(WATER_SURFACE_CONFIG_FILES
    ${WATER_SURFACE_SRC_DIR}/CMakeLists.txt
    ${WATER_SURFACE_SRC_DIR}/FileLists.cmake
    ${WATER_SURFACE_SRC_DIR}/LibLists.cmake)

SET(WATER_SURFACE_SHADER_FILES
    ${WATER_SURFACE_SRC_DIR}/resources/shaders/waterVelocity.vert
    ${WATER_SURFACE_SRC_DIR}/resources/shaders/waterVelocity.frag
    ${WATER_SURFACE_SRC_DIR}/resources/shaders/waterHeight.vert
    ${WATER_SURFACE_SRC_DIR}/resources/shaders/waterHeight.frag
    ${WATER_SURFACE_SRC_DIR}/resources/shaders/waterRender.vert
    ${WATER_SURFACE_SRC_DIR}/resources/shaders/waterRender.frag)

SET(WATER_SURFACE_SRC_FILES
    ${WATER_SURFACE_HEADERS}
    ${WATER_SURFACE_SOURCES}
    ${WATER_SURFACE_CONFIG_FILES}
    ${WATER_SURFACE_SHADER_FILES})

