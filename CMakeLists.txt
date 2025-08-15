cmake_minimum_required(VERSION 3.13)

project(magnum-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED
    GL
    MeshTools
    Shaders
    SceneGraph
    Trade
    Sdl2Application
    DebugTools
)
find_package(MagnumExtras REQUIRED Ui)
find_package(MagnumIntegration REQUIRED Bullet)
find_package(Bullet REQUIRED Dynamics)

add_executable(${PROJECT_NAME}
    src/ColoredDrawable.cpp
    src/TexturedDrawable.cpp
    src/RigidBody.cpp
    src/Player.cpp
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade
    Magnum::DebugTools
    MagnumExtras::Ui
    MagnumIntegration::Bullet
    Bullet::Dynamics
)
