BUILD_DIR := build

.PHONY: all build menuconfig run clean

all: build

build:
	cmake --build $(BUILD_DIR)

menuconfig:
	cmake -B $(BUILD_DIR) && cmake --build $(BUILD_DIR) --target menuconfig

run: build
	cmake --build $(BUILD_DIR) --target run
