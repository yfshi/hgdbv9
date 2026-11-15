#-------------------------------------------------------------------------
# CMake build for tecc_kms — TECC software SDF device simulator
#
# 产物（输出到 tecc_kms/bin/）：
#   libteccsdf.so   软件 SDF 设备动态库（客户端 fe-hsm-adapter 链接目标）
#   teccsdfinit     虚拟设备初始化工具（生成 KEK / SM2 密钥对）
#   teccsdftest     接口自测程序
#
# 源码：../src/（来自 GmSSL/SoftSDF，Apache-2.0）
# 依赖：GmSSL（SM2/SM3/SM4 国密算法）
#-------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
project(teccsdf C)

include(GNUInstallDirs)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall)
endif()

# ---- 源文件与头文件（位于 ../src）----
set(TECC_SDF_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set(TECC_SDF_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(SOFTSDF_SRC     ${TECC_SDF_SRC_DIR}/softsdf.c)
set(SOFTSDFINIT_SRC ${TECC_SDF_SRC_DIR}/softsdfinit.c)
set(SOFTSDFTEST_SRC ${TECC_SDF_SRC_DIR}/softsdftest.c)

# 产物输出到 tecc_kms/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

# ---- 定位 GmSSL（可通过 -DGmSSL_ROOT_DIR=/path/to/gmssl 指定）----
set(GmSSL_ROOT_DIR "" CACHE PATH "Root directory of a GmSSL static installation")

find_path(GmSSL_INCLUDE_DIR
    NAMES gmssl/sm2.h
    HINTS ${GmSSL_ROOT_DIR}
    PATH_SUFFIXES include)

set(_softsdf_find_library_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
if(NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .so .a)
endif()
find_library(GmSSL_LIBRARY
    NAMES gmssl libgmssl
    HINTS ${GmSSL_ROOT_DIR}
    PATH_SUFFIXES lib lib64)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${_softsdf_find_library_suffixes})

if(NOT GmSSL_INCLUDE_DIR)
    message(FATAL_ERROR "GmSSL headers not found. Set GmSSL_ROOT_DIR or GmSSL_INCLUDE_DIR.")
endif()
if(NOT GmSSL_LIBRARY)
    message(FATAL_ERROR "GmSSL static library not found. Set GmSSL_ROOT_DIR or GmSSL_LIBRARY.")
endif()

message(STATUS "GmSSL include dir: ${GmSSL_INCLUDE_DIR}")
message(STATUS "GmSSL library: ${GmSSL_LIBRARY}")

# ---- 动态库 libteccsdf.so ----
add_library(teccsdf SHARED ${SOFTSDF_SRC})
set_target_properties(teccsdf PROPERTIES OUTPUT_NAME "teccsdf")
target_include_directories(teccsdf PRIVATE
    ${GmSSL_INCLUDE_DIR} ${TECC_SDF_INC_DIR} ${TECC_SDF_SRC_DIR})
target_link_libraries(teccsdf PRIVATE ${GmSSL_LIBRARY})

# ---- 初始化工具 teccsdfinit ----
add_executable(teccsdfinit ${SOFTSDFINIT_SRC})
target_include_directories(teccsdfinit PRIVATE
    ${GmSSL_INCLUDE_DIR} ${TECC_SDF_INC_DIR} ${TECC_SDF_SRC_DIR})
target_link_libraries(teccsdfinit PRIVATE ${GmSSL_LIBRARY})

# ---- 接口自测 teccsdftest ----
add_executable(teccsdftest ${SOFTSDFTEST_SRC})
target_include_directories(teccsdftest PRIVATE
    ${GmSSL_INCLUDE_DIR} ${TECC_SDF_INC_DIR} ${TECC_SDF_SRC_DIR})
target_link_libraries(teccsdftest PRIVATE teccsdf ${GmSSL_LIBRARY})

# ---- 导出符号（仅导出 SDF_* 标准接口）----
if(APPLE)
    set_target_properties(teccsdf PROPERTIES
        LINK_FLAGS "-Wl,-exported_symbols_list,${TECC_SDF_SRC_DIR}/softsdf.exp -Wl,-x")
elseif(UNIX)
    set_target_properties(teccsdf PROPERTIES
        LINK_FLAGS "-Wl,--version-script=${TECC_SDF_SRC_DIR}/softsdf.lds -Wl,-x")
endif()

if(WIN32)
    target_link_libraries(teccsdf PRIVATE Ws2_32 Advapi32)
    target_link_libraries(teccsdfinit PRIVATE Ws2_32 Advapi32)
    target_link_libraries(teccsdftest PRIVATE Ws2_32 Advapi32)
    set_target_properties(teccsdf PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(BUILD_TESTING)
    add_test(NAME teccsdftest COMMAND teccsdftest)
endif()

install(TARGETS teccsdf teccsdfinit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib64)
install(FILES ${TECC_SDF_INC_DIR}/sdf.h ${TECC_SDF_INC_DIR}/sgd.h
    DESTINATION include/teccsdf)
