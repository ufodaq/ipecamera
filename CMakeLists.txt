project(ipecamera C)

set(RELEASE "0")
set(IPECAMERA_VERSION "0.0.1")
set(IPECAMERA_ABI_VERSION "0")

cmake_minimum_required(VERSION 2.6)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_definitions("-fPIC --std=gnu99 -Wall -O2 -gdwarf-2 -g3  -fno-omit-frame-pointer")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(UFODECODE ufodecode>=0.3 REQUIRED)

pkg_check_modules(PCILIB pcitool>=0.2 REQUIRED)
exec_program("pkg-config --variable=plugindir pcitool" OUTPUT_VARIABLE PCILIB_PLUGIN_DIR)

include(cmake/version.cmake)
VERSION_TO_VARS(${IPECAMERA_VERSION} IPECAMERA_VERSION_MAJOR IPECAMERA_VERSION_MINOR IPECAMERA_VERSION_MICRO)


add_subdirectory(apps)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${UFODECODE_INCLUDE_DIRS}
    ${PCILIB_INCLUDE_DIRS}
)

link_directories(
    ${UFODECODE_LIBRARY_DIRS}
    ${PCILIB_LIBRARY_DIRS}
)

set(HEADERS ${HEADERS} model.h cmosis.h base.h reader.h events.h data.h env.h private.h ipecamera.h version.h)

add_library(ipecamera SHARED model.c cmosis.c base.c reader.c events.c data.c env.c)

target_link_libraries(ipecamera ${PCILIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${UFODECODE_LIBRARIES} )

install(FILES ipecamera.h
    DESTINATION include
)

install(TARGETS ipecamera
    DESTINATION ${PCILIB_PLUGIN_DIR}
)

set(TARNAME "ipecamera")
set(PACKAGE_VERSION ${IPECAMERA_VERSION})
set(PACKAGE_NAME "${TARNAME}")
set(PACKAGE_TARNAME "${TARNAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://ufo.kit.edu/ufo/newticket")

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_PACKAGE_CONTACT "Suren A. Chilingaryan <csa@suren.me>")
if (${RELEASE} GREATER 0)
    set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}.${RELEASE}")
else (${RELEASE} GREATER 0)
    set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}")
endif (${RELEASE} GREATER 0)
set(CPACK_SOURCE_IGNORE_FILES "/.bzr/;CMakeFiles;_CPack_Packages;cmake_install.cmake;CPack.*.cmake;CMakeCache.txt;install_manifest.txt;config.h$;.pc$;Makefile;.tar.bz2$;~$;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")
include(CPack)

add_custom_target(dist_clean COMMAND ${CMAKE_MAKE_PROGRAM} clean WORKING_DIRECTORY ${CMAKE_CURRENT_DIR})
add_custom_target(dist DEPENDS dist_clean COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif(NOT DEFINED BIN_INSTALL_DIR)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR "${LIB_INSTALL_DIR}")
endif(NOT DEFINED LIB_INSTALL_DIR)

if(NOT DEFINED INCLUDE_INSTALL_DIR)
    set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
endif(NOT DEFINED INCLUDE_INSTALL_DIR)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ipecamera.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ipecamera.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ipecamera.spec.in ${CMAKE_CURRENT_BINARY_DIR}/ipecamera.spec)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/ipecamera.pc 
    DESTINATION ${LIB_INSTALL_DIR}/pkgconfig
)
