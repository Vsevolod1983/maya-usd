set(PXR_PACKAGE amdrprUsd)

amdrprUsd_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        mayaUsd
        sdf
        tf
        usd
        usdGeom
        usdMaya
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    CPPFILES
        plugin.cpp

    PRIVATE_HEADERS
        base/api.h

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)
