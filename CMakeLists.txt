cmake_minimum_required(VERSION 3.12)

project(MiniBlog VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(vendor/MiniSpring)

file(GLOB_RECURSE MINIBLOG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

file(GLOB_RECURSE MINIBLOG_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp"
)

add_executable(MiniBlog
    ${MINIBLOG_SOURCES}
    ${MINIBLOG_HEADERS}
)

target_include_directories(MiniBlog PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

target_link_libraries(MiniBlog PRIVATE
    MiniSpring::MiniSpring
)

target_compile_features(MiniBlog PRIVATE cxx_std_17)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT MiniBlog)

set_target_properties(MiniBlog PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set_target_properties(MiniBlog PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG}
        "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

add_custom_command(TARGET MiniBlog POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:MiniBlog>/resources"
)

if(MSVC)
    target_compile_options(MiniBlog PRIVATE /W3 /utf-8)
endif()