# 创建对象库
add_library(CoreObjects OBJECT
    ../Common/AngleLUT.cpp
    GameCore.cpp
    Tank.cpp
    ObstacleWall.cpp
    MapObstacleLoader.cpp
    Ai/AiController.cpp
    Ai/AiPerception.cpp
    Ai/AiSteering.cpp
    Ai/AiPathFollower.cpp
    Ai/NavigationGrid.cpp
    Ai/bt/TankBehaviorTree.cpp
)

# 设置包含目录
target_include_directories(CoreObjects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Ai
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
)

# MSVC defaults to system code page (e.g. 936); UTF-8 sources/comments otherwise mis-parse (C2447/C2059).
if(MSVC)
    target_compile_options(CoreObjects PRIVATE /utf-8)
endif()