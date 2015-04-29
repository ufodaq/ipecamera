include_directories(
    ${CMAKE_SOURCE_DIR}
    ${PCILIB_INCLUDE_DIRS}
)

link_directories(
    ${CMAKE_BINARY_DIR}
    ${PCILIB_LIBRARY_DIRS}
)


add_executable(grab grab.c)
target_link_libraries(grab ${PCILIB_LIBRARIES} ipecamera)
