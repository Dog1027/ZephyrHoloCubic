# 告訴 CMake 需要的最低版本
cmake_minimum_required(VERSION 3.20.0)

# 引入 Zephyr 建構系統
find_package(Zephyr)

# 定義您的應用程式專案名稱
project(my_zephyr_app)

# 將 src/ 目錄下的所有原始碼加入到建構目標 'app' 中
target_sources(app PRIVATE
  src/main.c
  src/tool.c
  src/gui.c
)
