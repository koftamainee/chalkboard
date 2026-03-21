cmake_minimum_required(VERSION 3.20)
project(chalkboard VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHALKBOARD_BUILD_VIEWER "Build the chalkboard viewer executable" OFF)
option(CHALKBOARD_BUILD_EXAMPLES "Build the chalkboard examples" OFF)

include(FetchContent)
set(HTTPLIB_REQUIRE_OPENSSL OFF)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG master
)
FetchContent_MakeAvailable(httplib)

add_library(chalkboard STATIC
        src/latex_list.cpp
        src/reporter.cpp
        src/publisher.cpp
        src/task.cpp
)
target_include_directories(chalkboard
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(chalkboard
        PUBLIC
        httplib::httplib
)
target_compile_features(chalkboard PUBLIC cxx_std_20)
add_library(chalkboard::chalkboard ALIAS chalkboard)

if (CHALKBOARD_BUILD_VIEWER)
    add_executable(chalkboard_viewer
            viewer/viewer.cpp
    )
    target_link_libraries(chalkboard_viewer
            PRIVATE
            httplib::httplib
    )
    target_compile_features(chalkboard_viewer PRIVATE cxx_std_20)
endif ()

if (CHALKBOARD_BUILD_EXAMPLES)
    foreach (name IN ITEMS task1_pythagorean task2_euler task3_quadratic)
        set(src ${CMAKE_CURRENT_SOURCE_DIR}/examples/${name}.cpp)
        if (EXISTS ${src})
            add_executable(example_${name} ${src})
            target_link_libraries(example_${name} PRIVATE chalkboard::chalkboard)
            target_compile_features(example_${name} PRIVATE cxx_std_20)
            target_precompile_headers(example_${name} PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/examples/common.h
            )
        endif ()
    endforeach ()
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS chalkboard
        EXPORT chalkboardTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT chalkboardTargets
        FILE chalkboardTargets.cmake
        NAMESPACE chalkboard::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/chalkboard
)
write_basic_package_version_file(
        chalkboardConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/chalkboardConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/chalkboardConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/chalkboard
)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/chalkboardConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/chalkboardConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/chalkboard
)