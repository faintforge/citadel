cmake_minimum_required(VERSION 3.31)
project(citadel)

add_subdirectory("spire/")

file(GLOB_RECURSE SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_library(${PROJECT_NAME} ${SOURCE})

# Make __FILE__ macro return a relative path.
foreach(f IN LISTS SOURCE)
    file(RELATIVE_PATH b ${CMAKE_SOURCE_DIR} ${f})
    set_source_files_properties(${f} PROPERTIES COMPILE_DEFINITIONS "__FILE__=\"${b}\"")
endforeach()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    C_STANDARD "99"
    POSITION_INDEPENDENT_CODE true
    C_STANDARD_REQUIRED true
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    COMPILE_FLAGS "-Wall -Wextra -Wno-builtin-macro-redefined -Wpedantic"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/"
)

target_link_libraries(${PROJECT_NAME} spire)
if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} xcb xcb-icccm xcb-keysyms X11 X11-xcb EGL m)
endif ()


if (EMSCRIPTEN)
    execute_process(COMMAND emcc --cflags OUTPUT_VARIABLE EM_CFLAGS)
    get_target_property(CFLAGS ${CMAKE_PROJECT_NAME} COMPILE_FLAGS)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
        COMPILE_FLAGS "${CFLAGS} ${EM_CFLAGS}"
    )
    target_link_options(${CMAKE_PROJECT_NAME}
        PRIVATE "-sALLOW_MEMORY_GROWTH=1"
    )
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()
