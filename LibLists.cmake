# Qt
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})

SET(WATER_SURFACE_LIBRARIES
    ${QT_LIBRARIES}
    CellarWorkbench
    MediaWorkbench
    PropRoom2D
    Scaena
)
    
SET(WATER_SURFACE_INCLUDE_DIRS
    ${WATER_SURFACE_SRC_DIR}
    ${WATER_SURFACE_INSTALL_PREFIX}/include/CellarWorkbench
    ${WATER_SURFACE_INSTALL_PREFIX}/include/MediaWorkbench
    ${WATER_SURFACE_INSTALL_PREFIX}/include/PropRoom2D
    ${WATER_SURFACE_INSTALL_PREFIX}/include/Scaena)
