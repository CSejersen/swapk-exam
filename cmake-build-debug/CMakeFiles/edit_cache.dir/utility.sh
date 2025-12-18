set -e

cd /Users/sejersen/Documents/AU/5-semester/swapk-exam/cmake-build-debug
/opt/homebrew/bin/ccmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
