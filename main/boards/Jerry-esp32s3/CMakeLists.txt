# Jerry ESP32-S3开发板CMakeLists.txt

# 获取当前目录路径
get_filename_component(BOARD_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

# 添加开发板源文件
list(APPEND BOARD_SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/jerry_esp32s3_board.cc
    ${CMAKE_CURRENT_LIST_DIR}/jerry_emoji_display.cc
)

# 添加自定义GIF资源文件
list(APPEND BOARD_SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/../../newfunction/gif/happy.c
    ${CMAKE_CURRENT_LIST_DIR}/../../newfunction/gif/natural.c
)

# 设置包含目录
list(APPEND BOARD_INCLUDE_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../newfunction/gif
)

# 更新全局变量
set(BOARD_SOURCES ${BOARD_SOURCES} PARENT_SCOPE)
set(BOARD_INCLUDE_DIRS ${BOARD_INCLUDE_DIRS} PARENT_SCOPE)