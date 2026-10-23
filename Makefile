# Atalhos para o CMake. Uso:
#   make run                         compila (Debug) e executa
#   make debug                       compila e abre no gdb
#   make run QT_PATH=~/Qt/6.8.0/gcc_64   se o Qt não estiver no PATH do sistema
#   make package                     gera os pacotes do sistema atual em dist/ (scripts/package.sh)
#   make appimage | make deb         só o AppImage / só o .deb (Linux)

BUILD_DIR  ?= build
BUILD_TYPE ?= Debug
QT_PATH    ?=
JOBS       ?= $(shell nproc 2>/dev/null || echo 4)

CMAKE_ARGS := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
ifneq ($(strip $(QT_PATH)),)
CMAKE_ARGS += -DCMAKE_PREFIX_PATH=$(QT_PATH)
endif

# No macOS o executável fica dentro do bundle myqtapp.app
ifeq ($(shell uname -s),Darwin)
APP_EXE := ./myqtapp.app/Contents/MacOS/myqtapp
DEBUGGER := lldb
else
APP_EXE := ./myqtapp
DEBUGGER := gdb -q
endif

ifeq ($(shell command -v cmake 2>/dev/null),)
$(error cmake não encontrado. Instale com: sudo apt install cmake build-essential qt6-base-dev libssl-dev gdb)
endif

.PHONY: all configure build run debug clean rebuild package appimage deb

all: build

$(BUILD_DIR)/CMakeCache.txt:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_ARGS)

configure:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_ARGS)

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) -j $(JOBS)

run: build
	cd $(BUILD_DIR) && $(APP_EXE)

debug: build
	cd $(BUILD_DIR) && $(DEBUGGER) $(APP_EXE)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

package:
	QT_PATH="$(QT_PATH)" JOBS="$(JOBS)" scripts/package.sh

appimage deb:
	QT_PATH="$(QT_PATH)" JOBS="$(JOBS)" scripts/package.sh $@
