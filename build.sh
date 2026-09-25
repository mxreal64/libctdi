#!/usr/bin/env bash

set -euo pipefail

CXX=g++
CXXFLAGS="-std=c++26 -freflection -fexpansion-statements"
BUILD_DIR="build"
MODULE="ctdi.cppm"

mkdir -p "${BUILD_DIR}"

echo "==> Compiling module interface: ${MODULE_SRC}"
${CXX} ${CXXFALGS} -fmodules-ts \
  -c "${MODULE}" -o "${BUILD_DIR}/ctdi.o"

echo "==> Done."
