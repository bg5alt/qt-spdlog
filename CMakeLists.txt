cmake_minimum_required(VERSION 3.16)

project(Reflect LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.15.2/5.15.2/msvc2019_64")

# 设置 debug 库 带 d
set(CMAKE_DEBUG_POSTFIX d)
set(DEBUG_POSTFIX "")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG_POSTFIX "d")
endif()

set(BUILD_ARCH "x64")




# 设置查找路径 (鸿蒙下 必须设置 否者会报 找不到 qt sdk )
list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})


# 查找 版本
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

# 设置 qt 库
if ("${QT_VERSION_MAJOR}" MATCHES "5")
    set(PackageLibs Core Gui Widgets Network Svg Concurrent)
elseif ("${QT_VERSION_MAJOR}" MATCHES "6")
    set(PackageLibs Core Gui Widgets Network Svg Concurrent  SvgWidgets Core5Compat )
endif ()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${PackageLibs}  LinguistTools REQUIRED)

# 链接 模块
set(QtLinkLibs "")
foreach(Lib ${PackageLibs})
    list(APPEND QtLinkLibs Qt${QT_VERSION_MAJOR}::${Lib})
endforeach(Lib)

add_definitions(-DTOOL_LIBRARY)

set(3RD_INCLUDE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include"
)
set(3RD_LIBS
        "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/lib/spdlog.lib"
)


# 设置 头文件目录 (这种设置方式 使用头文件 需要添加目录) 比如 "a/b.h"
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# GLOB_RECURSE 会递归查找所有子目录的文件  GLOB 只会查找当前目录
set( INCLUDE_FILES
        "estream.h"
        "logmanager_p.h"
        "logmanager.h"
        "logstream.h"
)
set( SOURCE_FILES
        "logmanager.cpp"
        "logstream.cpp"
        "main.cpp"
)

# 强制设置源文件编码为 UTF-8（针对不同编译器）
if(MSVC)
    # Visual Studio 编译器：添加 /utf-8 选项（强制源文件和执行字符集为 UTF-8）
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
else()
    # GCC/Clang 编译器：指定源文件编码为 UTF-8，执行字符集为 UTF-8
    add_compile_options(-finput-charset=UTF-8)
    add_compile_options(-fexec-charset=UTF-8)
endif()

#发布目录install目录设置
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}")
#设置语言编译输出目录
set(LANG_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}/lang)
set_source_files_properties(${SOURCES_TS} PROPERTIES OUTPUT_LOCATION ${EXECUTABLE_OUTPUT_PATH}/lang)

#添加资源与编译语言
#qt_add_resources(QRC_FILES ${SOURCES_QRC})
#qt_add_translation(QM_FILES ${SOURCES_TS})

# 添加ui 文件路径搜索路径(如不添加则出现 找不到ui_*.h 头文件)
#list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS ${SOURCES_UI})


#if (WIN32)
#    enable_language(RC)  # 启用资源编译器支持[[7]]
#endif()

#Windows 图标设置
if (WIN32)
    set(APP_RC resource/dzh.rc)
    set(ICON_NAME "")
endif()

# 生成 库 或 可执行文件
if ("${QT_VERSION_MAJOR}" MATCHES "5")
    add_executable(
            ${PROJECT_NAME}
            ${INCLUDE_FILES}
            ${SOURCE_FILES}
            ${QRC_FILES}
            ${QM_FILES}
            ${SOURCES_TS}
            ${SOURCES_QRC}
            ${SOURCES_UI}
    )
elseif("${QT_VERSION_MAJOR}" MATCHES "6")
    qt_add_executable(
            ${PROJECT_NAME}
            ${INCLUDE_FILES}
            ${SOURCE_FILES}
            ${QRC_FILES}
            ${QM_FILES}
            ${SOURCES_TS}
            ${SOURCES_QRC}
            ${SOURCES_UI}
            ${APP_RC}
    )
endif()


# 关联 头文件
target_include_directories(${PROJECT_NAME} PRIVATE ${3RD_INCLUDE_DIR})
# 关联 库
target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        ${QtLinkLibs}
        ${3RD_LIBS}
)


# 设置 QM 文件的输出路径
foreach(QM_FILE ${QM_FILES})
    get_filename_component(QM_BASENAME ${QM_FILE} NAME)
    set_source_files_properties(${QM_FILE} PROPERTIES OUTPUT_LOCATION ${LANG_OUTPUT_PATH})
endforeach()


#Windows dll关联

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif()
    endif()

    foreach(QT_LIB ${PackageLibs} )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(QT_LIB)

    #    message(STATUS "Begin Deployqt Project ...")
    find_program(DEPLOYQT_EXECUTABLE NAMES windeployqt macdeployqt HINTS "${QT_INSTALL_PATH}/bin")
    # 自动调用 windeployqt
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${DEPLOYQT_EXECUTABLE} "$<TARGET_FILE:${PROJECT_NAME}>") #--qmldir ${CMAKE_SOURCE_DIR} --$<LOWER_CASE:$<CONFIG>>

    message(STATUS "End deployqt Project ...")
    #Release版本屏蔽mingw编译环境下，应用程序带CMD窗口的设置
    if (CMAKE_BUILD_TYPE MATCHES "Release")
        if(MSVC)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                    WIN32_EXECUTABLE YES
                    LINK_FLAGS "/ENTRY:mainCRTStartup"
            )
        elseif(CMAKE_COMPILER_IS_GNUCXX)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested
        endif()
    endif()

endif()


