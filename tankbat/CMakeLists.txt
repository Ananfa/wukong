cmake_minimum_required(VERSION 3.20)
project(Tankbat LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置为动态库
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)  # 自动导出所有符号
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/UnityInterface)

# 添加子目录
add_subdirectory(Core)
add_subdirectory(UnityInterface)

# 创建DLL
add_library(Tankbat SHARED
    $<TARGET_OBJECTS:CoreObjects>
    $<TARGET_OBJECTS:InterfaceObjects>
)

# 设置输出名
set_target_properties(Tankbat PROPERTIES
    OUTPUT_NAME "TankbattleNative"
    PREFIX ""  # 不添加lib前缀
    SUFFIX ".dll"  # Windows DLL后缀
)

# 链接库
target_link_libraries(Tankbat)

# Unity兼容性设置
if(MSVC)
    target_compile_options(Tankbat PRIVATE
        /utf-8
        /MD  # 使用动态运行时库，与Unity匹配
        /EHsc  # 启用C++异常处理
        /W3  # 警告等级
        /DWIN32
        /D_WINDOWS
        /D_UNICODE
        /DUNICODE
    )
    
    # DLL导出设置
    set_target_properties(Tankbat PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    
    # 添加定义，确保正确导出
    target_compile_definitions(Tankbat PRIVATE
        UNITYNATIVEPLUGIN_EXPORTS
    )
endif()