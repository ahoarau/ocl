# Locate KDL install directory

# This module defines
# KDL_HOME where to find include, lib, bin, etc.
# KDL_FOUND, is set to true

INCLUDE (${PROJ_SOURCE_DIR}/config/FindPkgConfig.cmake)

IF ( CMAKE_PKGCONFIG_EXECUTABLE )

    MESSAGE( "Using pkgconfig" )
    
    SET(ENV{PKG_CONFIG_PATH} "${KDL_INSTALL}/lib/pkgconfig/")
    MESSAGE( "Setting environment of PKG_CONFIG_PATH to: $ENV{PKG_CONFIG_PATH}")
    PKGCONFIG( "kdl >= 0.1" KDL_FOUND KDL_INCLUDE_DIRS KDL_DEFINES KDL_LINK_DIRS KDL_LIBS )

    IF( KDL_FOUND )
        MESSAGE("   Includes in: ${KDL_INCLUDE_DIRS}")
        MESSAGE("   Libraries in: ${KDL_LINK_DIRS}")
        MESSAGE("   Libraries: ${KDL_LIBS}")
        MESSAGE("   Defines: ${KDL_DEFINES}")

	INCLUDE_DIRECTORIES( ${KDL_INCLUDE_DIRS} )
        LINK_DIRECTORIES( ${KDL_LINK_DIRS} )

    ENDIF ( KDL_FOUND )

ELSE  ( CMAKE_PKGCONFIG_EXECUTABLE )

    # Can't find pkg-config -- have to search manually
    MESSAGE( FATAL_ERROR "Can't find KDL")

ENDIF ( CMAKE_PKGCONFIG_EXECUTABLE )
