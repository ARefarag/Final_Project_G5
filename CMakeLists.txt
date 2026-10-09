cmake_minimum_required(VERSION 3.20)
project(GameAndGo LANGUAGES CXX)

# ============================================================================
# GAME&GO - CMAKE CONFIGURATION
# OWNER: MEMBER 5
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Window and Input
# ADDED: Prefer the installed GLFW package. The Windows one-click bootstrap
# installs GLFW through MSYS2, avoiding a separate GitHub FetchContent download.
find_package(glfw3 CONFIG QUIET)
if(TARGET glfw)
    set(GAMEANDGO_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(GAMEANDGO_GLFW_TARGET glfw3::glfw)
elseif(WIN32)
    message(FATAL_ERROR "GLFW was not found. On Windows, start GameAndGo-Windows.bat to install required packages.")
else()
    # Keep FetchContent as a fallback for Linux/macOS developer machines.
    include(FetchContent)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.3.8)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
    set(GAMEANDGO_GLFW_TARGET glfw)
endif()

find_package(OpenGL REQUIRED)

# Dear ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

include_directories(include)

# Compile all modules
file(GLOB_RECURSE APP_SOURCES
    "src/Core/*.cpp"
    "src/Domain/*.cpp"
    "src/Database/*.cpp"
    "src/Presentation/*.cpp"
)

add_executable(App main.cpp ${APP_SOURCES} ${IMGUI_SOURCES})

target_include_directories(App PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends include)

# Link OpenGL, GLFW, and PostgreSQL (libpqxx + libpq)
target_link_libraries(App PRIVATE OpenGL::GL ${GAMEANDGO_GLFW_TARGET} pqxx pq)