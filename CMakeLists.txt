project(ipecamera)

set(IPECAMERA_VERSION "0.0.1")
set(IPECAMERA_ABI_VERSION "0")

cmake_minimum_required(VERSION 2.6)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(UFODECODE ufodecode>=0.3 REQUIRED)

pkg_check_modules(PCILIB pcitool>=0.2 REQUIRED)
exec_program("pkg-config --variable=plugindir pcitool" OUTPUT_VARIABLE PCILIB_PLUGIN_DIR)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${UFODECODE_INCLUDE_DIRS}
    ${PCILIB_INCLUDE_DIRS}
)

link_directories(
    ${UFODECODE_LIBRARY_DIRS}
    ${PCILIB_LIBRARY_DIRS}
)

set(HEADERS ${HEADERS} model.h cmosis.h base.h reader.h events.h data.h private.h ipecamera.h)

add_library(ipecamera SHARED model.c cmosis.c base.c reader.c events.c data.c)

target_link_libraries(ipecamera ${PCILIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(FILES ipecamera.h
    DESTINATION include
)

install(TARGETS ipecamera
    DESTINATION ${PCILIB_PLUGIN_DIR}
)

