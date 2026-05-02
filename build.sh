#!/bin/bash
# 梅花易数 AI 大师 - 一键构建脚本
# 用法: ./build.sh [release|debug]

set -e

BUILD_TYPE="${1:-release}"
BUILD_DIR="build"

echo "========================================"
echo "  梅花易数 AI 大师 - 构建"
echo "  模式: $BUILD_TYPE"
echo "========================================"

# 检查依赖
check_dep() {
    if ! command -v "$1" &> /dev/null; then
        echo "❌ 缺少依赖: $1"
        echo "   安装: $2"
        exit 1
    fi
    echo "✅ $1 已安装"
}

echo ""
echo "检查依赖..."
check_dep "cmake" "sudo apt install cmake"
check_dep "g++" "sudo apt install g++"

# 检查libcurl
if ! pkg-config --exists libcurl 2>/dev/null && ! [ -f "/usr/include/curl/curl.h" ]; then
    echo "❌ 缺少依赖: libcurl"
    echo "   安装: sudo apt install libcurl4-openssl-dev"
    exit 1
fi
echo "✅ libcurl 已安装"

# 检查SQLite3
if ! [ -f "/usr/include/sqlite3.h" ] && ! dpkg -l | grep -q libsqlite3-dev; then
    echo "❌ 缺少依赖: libsqlite3-dev"
    echo "   安装: sudo apt install libsqlite3-dev"
    exit 1
fi
echo "✅ sqlite3 已安装"

echo ""
echo "开始构建..."

# 创建构建目录
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake配置
if [ "$BUILD_TYPE" = "debug" ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug ..
else
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

# 编译
cmake --build . -j$(nproc)

echo ""
echo "========================================"
echo "  ✅ 构建成功！"
echo ""
echo "  运行方式:"
echo "    cd $BUILD_DIR && ./zhouyi-master"
echo ""
echo "  或直接:"
echo "    ./$BUILD_DIR/zhouyi-master"
echo "========================================"
