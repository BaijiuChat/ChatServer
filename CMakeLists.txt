cmake_minimum_required(VERSION 3.21)
project(ChatServer LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 将文件从源目录拷贝到构建目录（构建时立即执行）
configure_file(
        ${CMAKE_SOURCE_DIR}/config.ini  # 源文件
        ${CMAKE_BINARY_DIR}/bin/config.ini     # 目标路径
        COPYONLY                                 # 直接复制，不替换变量
)

# 包含 vcpkg 工具链
set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# 查找 vcpkg 安装的库
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)

# 设置头文件和源文件目录
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

# 收集头文件和源文件
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${INCLUDE_DIR}/*.h" "${INCLUDE_DIR}/*.hpp")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.cc")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
        ${INCLUDE_DIR}
        "D:/jsoncpp-0.y.z/include"
)

# 配置 Debug 和 Release 特定的包含目录和库
target_include_directories(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:D:/MySQL\ Server\ 8.4/Connector/C++-debug-9.3.0/include>
        $<$<CONFIG:Release>:D:/MySQL\ Server\ 8.4/Connector/C++-release-9.3.0/include>
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
        Boost::system
        Boost::filesystem
        gRPC::grpc++
        protobuf::libprotobuf
        hiredis::hiredis
        redis++::redis++
        # jsoncpp 和 MySQL Connector 库
        $<$<CONFIG:Debug>:D:/jsoncpp-0.y.z/makefiles/vs71/x64/libjson/json_vc71_libmtd.lib>
        $<$<CONFIG:Release>:D:/jsoncpp-0.y.z/makefiles/vs71/x64/libjson/json_vc71_libmt.lib>
        $<$<CONFIG:Debug>:D:/MySQL\ Server\ 8.4/Connector/C++-debug-9.3.0/lib64/debug/vs14/mysqlcppconn.lib>
        $<$<CONFIG:Debug>:D:/MySQL\ Server\ 8.4/Connector/C++-debug-9.3.0/lib64/debug/vs14/mysqlcppconnx.lib>
        $<$<CONFIG:Release>:D:/MySQL\ Server\ 8.4/Connector/C++-release-9.3.0/lib64/vs14/mysqlcppconn.lib>
        $<$<CONFIG:Release>:D:/MySQL\ Server\ 8.4/Connector/C++-release-9.3.0/lib64/vs14/mysqlcppconnx.lib>
)

# 设置 Debug 和 Release 特定的库目录
target_link_directories(${PROJECT_NAME} PRIVATE
        D:/jsoncpp-0.y.z/makefiles/vs71/x64/libjson
        $<$<CONFIG:Debug>:D:/MySQL\ Server\ 8.4/Connector/C++-debug-9.3.0/lib64/debug/vs14>
        $<$<CONFIG:Release>:D:/MySQL\ Server\ 8.4/Connector/C++-release-9.3.0/lib64/vs14>
)

# 确保 MSVC 使用正确的运行时库
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            "/utf-8"
            $<$<CONFIG:Debug>:/MDd>
            $<$<CONFIG:Release>:/MD>
    )
endif()

# 启用文件夹组织（Visual Studio 和其他 IDE）
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${INCLUDE_DIR} PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE ${SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCES})

# 复制必要的 DLL（可选，根据需要启用）
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "<path_to_dll>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
# )