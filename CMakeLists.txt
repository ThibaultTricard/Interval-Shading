cmake_minimum_required (VERSION 3.20)
project( "Interval" )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(LAVACAKE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(LavaCake)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/envmaps DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/objects DESTINATION ${CMAKE_CURRENT_BINARY_DIR})



add_compile_definitions(PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/")

# --- External dependencies (glfw, glm, imgui, stb) ---
include(FetchContent)

if(WIN32)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glm)
else()
    find_package(glfw3 REQUIRED)
    find_package(glm REQUIRED)
endif()

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_glfw STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
target_include_directories(imgui_glfw PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_glfw PUBLIC glfw)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})

# --- Executables ---

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/crystal)
add_executable( crystal  "./src/crystal.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
target_link_libraries( crystal LavaCake glfw glm::glm imgui_glfw stb_image )




file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/singleTet)
add_executable( singleTet  "./src/singleTet.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
target_link_libraries( singleTet LavaCake glfw glm::glm )




file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/armadillo)
add_executable( armadillo  "./src/armadillo.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
target_link_libraries( armadillo LavaCake glfw glm::glm imgui_glfw )




file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/bunny)
add_executable( bunny  "./src/bunny.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
target_link_libraries( bunny LavaCake glfw glm::glm imgui_glfw )




file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/asteroid)
add_executable( asteroids  "./src/asteroids.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
target_link_libraries( asteroids LavaCake glfw glm::glm )


file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/jet)
add_executable( jet  "./src/jet.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
target_link_libraries( jet LavaCake glfw glm::glm imgui_glfw )
