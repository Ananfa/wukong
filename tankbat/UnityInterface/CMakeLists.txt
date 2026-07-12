add_library(InterfaceObjects OBJECT
    TankBattleWrapper.cpp
)

target_include_directories(InterfaceObjects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
)

if(MSVC)
    target_compile_options(InterfaceObjects PRIVATE /utf-8)
endif()