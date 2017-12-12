NO_COLOR=\x1b[0m
OK_COLOR=\x1b[32;01m
ERROR_COLOR=\x1b[31;01m
WARN_COLOR=\x1b[33;01m

HOOKZZ_NAME := hookzz
HOOKZZ_PATH := $(abspath .)
LOCAL_PATH := $(abspath .)
OUTPUT_DIR := $(abspath build)

CFLAGS ?= -O0 -g
CXXFLAGS = $(CFLAGS) -stdlib=libc++ -std=c++11 -gmodules
LDFLAGS ?=
LIBS_CFLAGS ?= -fPIC

HOST ?= $(shell uname -s)
HOST_ARCH ?= $(shell uname -m)

ifeq ($(HOST), Darwin)

endif

# ------------ hookzz make env ---------------

HOOKZZ_INCLUDE := $(HOOKZZ_PATH)/include \
			$(HOOKZZ_PATH)/src

HOOKZZ_SRC_FILES := $(wildcard $(HOOKZZ_PATH)/src/*.c) \
			$(wildcard $(HOOKZZ_PATH)/src/platforms/backend-posix/*.c)

BACKEND ?= ios
ARCH ?= arm64

ifeq ($(BACKEND), ios)
	ifeq ($(ARCH), arm)
		ZZ_ARCH := armv7
	else ifeq ($(ARCH), arm64)
		ZZ_ARCH := arm64
	endif

	ZZ_BACKEND := ios
	ZZ_GXX_BIN := $(shell xcrun --sdk iphoneos --find clang++)
	ZZ_GCC_BIN := $(shell xcrun --sdk iphoneos --find clang)
	ZZ_SDK_ROOT := $(shell xcrun --sdk iphoneos --show-sdk-path)
	ZZ_AR_BIN := $(shell which ar)
	ZZ_RANLIB_BIN := $(shell which ranlib)
	ZZ_GCC_SOURCE := $(ZZ_GCC_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_GXX_SOURCE := $(ZZ_GXX_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_GCC_TEST := $(ZZ_GCC_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_GXX_TEST := $(ZZ_GXX_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_CFLAGS := -g -fPIC -shared -dynamiclib
	ZZ_DLL := lib$(HOOKZZ_NAME).dylib
	CFLAGS += -arch $(ZZ_ARCH)

	HOOKZZ_SRC_FILES += $(wildcard $(HOOKZZ_PATH)/src/platforms/backend-darwin/*.c)

else ifeq ($(BACKEND), macos)
	ifeq ($(ARCH), x86)
		ZZ_ARCH := i386
	else ifeq ($(ARCH), x86_64)
		ZZ_ARCH := x86_64
	endif

	ZZ_BACKEND := macos
	ZZ_GXX_BIN := $(shell xcrun --sdk macosx --find clang++)
	ZZ_GCC_BIN := $(shell xcrun --sdk macosx --find clang)
	ZZ_SDK_ROOT := $(shell xcrun --sdk macosx --show-sdk-path)
	ZZ_AR_BIN := $(shell which ar)
	ZZ_RANLIB_BIN := $(shell which ranlib)
	ZZ_GCC_SOURCE := $(ZZ_GCC_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_GXX_SOURCE := $(ZZ_GXX_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_GCC_TEST := $(ZZ_GCC_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_GXX_TEST := $(ZZ_GXX_BIN) -isysroot $(ZZ_SDK_ROOT)
	ZZ_CFLAGS := -g -fPIC -shared -dynamiclib
	ZZ_DLL := lib$(HOOKZZ_NAME).dylib
	CFLAGS += -arch $(ZZ_ARCH)
	
	HOOKZZ_SRC_FILES += $(wildcard $(HOOKZZ_PATH)/src/platforms/backend-darwin/*.c)

else ifeq ($(BACKEND), android)
	ZZ_BACKEND := android

	ifeq ($(ARCH), arm)
		ZZ_ARCH := armv7
		ZZ_API_LEVEL := android-19
		ZZ_CROSS_PREFIX := arm-linux-androideabi-
		ZZ_BIN_CROSS_PREFIX := arm-linux-androideabi-
	else ifeq ($(ARCH), arm64)
		ZZ_ARCH := arm64
		ZZ_API_LEVEL := android-21
		ZZ_CROSS_PREFIX := aarch64-linux-android-
		ZZ_BIN_CROSS_PREFIX := aarch64-linux-android-
	else ifeq ($(ARCH), x86)
		ZZ_ARCH := x86
		ZZ_API_LEVEL := android-21
		ZZ_CROSS_PREFIX := x86-
		ZZ_BIN_CROSS_PREFIX := i686-linux-android-

	endif

	HOST_DIR := $(shell echo $(HOST) | tr A-Z a-z)-$(HOST_ARCH)
	ZZ_NDK_HOME := $(shell dirname `which ndk-build`)
	ZZ_SDK_ROOT := $(ZZ_NDK_HOME)/platforms/$(ZZ_API_LEVEL)/arch-$(ARCH)
	ZZ_GCC_BIN := $(ZZ_NDK_HOME)/toolchains/$(ZZ_CROSS_PREFIX)4.9/prebuilt/$(HOST_DIR)/bin/$(ZZ_BIN_CROSS_PREFIX)gcc
	ZZ_GXX_BIN := $(ZZ_NDK_HOME)/toolchains/$(ZZ_CROSS_PREFIX)4.9/prebuilt/$(HOST_DIR)/bin/$(ZZ_BIN_CROSS_PREFIX)g++
	ZZ_AR_BIN := $(ZZ_NDK_HOME)/toolchains/$(ZZ_CROSS_PREFIX)4.9/prebuilt/$(HOST_DIR)/bin/$(ZZ_BIN_CROSS_PREFIX)ar
	ZZ_RANLIB_BIN := $(ZZ_NDK_HOME)/toolchains/$(ZZ_CROSS_PREFIX)4.9/prebuilt/$(HOST_DIR)/bin/$(ZZ_BIN_CROSS_PREFIX)ranlib
	ZZ_GCC_SOURCE := $(ZZ_GCC_BIN) --sysroot=$(ZZ_SDK_ROOT)
	ZZ_GXX_SOURCE := $(ZZ_GXX_BIN) --sysroot=$(ZZ_SDK_ROOT)
	ZZ_GCC_TEST := $(ZZ_GCC_BIN) --sysroot=$(ZZ_SDK_ROOT)
	ZZ_GXX_TEST := $(ZZ_GXX_BIN) --sysroot=$(ZZ_SDK_ROOT)
	ZZ_CFLAGS := -g -fPIC -shared
	ZZ_DLL := lib$(HOOKZZ_NAME).so

	HOOKZZ_SRC_FILES += $(wildcard $(HOOKZZ_PATH)/src/platforms/backend-linux/*.c)

endif

ifeq ($(ARCH), arm)
HOOKZZ_SRC_FILES += $(wildcard $(HOOKZZ_PATH)/src/platforms/arch-arm/*.c) \
			$(wildcard $(HOOKZZ_PATH)/src/platforms/backend-arm/*.c)
else ifeq ($(ARCH), arm64)
HOOKZZ_SRC_FILES += $(wildcard $(HOOKZZ_PATH)/src/platforms/backend-arm64/*.S)
HOOKZZ_SRC_FILES += $(wildcard $(HOOKZZ_PATH)/src/platforms/arch-arm64/*.c) \
			$(wildcard $(HOOKZZ_PATH)/src/platforms/backend-arm64/*.c)
endif

# ------------ hookzz make env ---------------

# ------------ kitzz make env ---------------

KITZZ_PATH := /Users/jmpews/project/kitzz

KITZZ_INCLUDE := $(KITZZ_PATH) \
			$(KITZZ_PATH)/include

KITZZ_FILES_PATH := $(KITZZ_PATH)/CommonKit \
			$(KITZZ_PATH)/PosixKit \
			$(KITZZ_PATH)/MachoKit \
			$(KITZZ_PATH)/DarwinKit

KITZZ_FILES_SUFFIX := %.cpp %.c

define walk
    $(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

KITZZ_ALLFILES := $(foreach src_path,$(KITZZ_FILES_PATH), $(call walk,$(src_path),*.*) )
#$(warning KITZZ_ALLFILES $(KITZZ_ALLFILES))
KITZZ_FILE_LIST  := $(filter $(KITZZ_FILES_SUFFIX),$(KITZZ_ALLFILES))
KITZZ_SRC_FILES := $(KITZZ_FILE_LIST:$(LOCAL_PATH)/%=%)
#$(warning KITZZ_SRC_FILES= $(KITZZ_SRC_FILES))

# ------------ kitzz make env end ---------------

HOOKZZ_INCLUDE += $(KITZZ_INCLUDE)

HOOKZZ_C_CPP_SRC_FILES := $(filter %.c,$(HOOKZZ_SRC_FILES)) \
				$(KITZZ_SRC_FILES)
HOOKZZ_ASM_SRC_FILES := $(filter %.S,$(HOOKZZ_SRC_FILES))

HOOKZZ_C_CPP_OBJ_FILES := $(HOOKZZ_C_CPP_SRC_FILES:.c=.o)
HOOKZZ_ASM_OBJ_FILES := $(HOOKZZ_ASM_SRC_FILES:.S=.o)

HOOKZZ_SRC_OBJ_FILES := $(HOOKZZ_C_CPP_OBJ_FILES) $(HOOKZZ_ASM_OBJ_FILES)

HOOKZZ_INCLUDE := $(foreach n, $(HOOKZZ_INCLUDE), -I$(n))

$(HOOKZZ_NAME) : $(HOOKZZ_SRC_OBJ_FILES)
	@mkdir -p $(OUTPUT_DIR)
	@rm -rf $(OUTPUT_DIR)/*

	@$(ZZ_GCC_SOURCE) $(ZZ_CFLAGS) $(CFLAGS) $(LDFLAGS) $(HOOKZZ_INCLUDE) $(HOOKZZ_SRC_OBJ_FILES) -o $(OUTPUT_DIR)/$(ZZ_DLL)
	@$(ZZ_AR_BIN) -rcs $(OUTPUT_DIR)/lib$(HOOKZZ_NAME).static.a $(ZZ_OBJS)

	@echo "$(OK_COLOR)build success for $(ARCH)-$(BACKEND)-hookzz! $(NO_COLOR)"

$(HOOKZZ_C_CPP_OBJ_FILES): %.o : %.c
	@$(ZZ_GCC_SOURCE) $(CFLAGS) $(HOOKZZ_INCLUDE) -c $< -o $@
	@echo "$(OK_COLOR)generate [$@]! $(NO_COLOR)"

$(HOOKZZ_ASM_OBJ_FILES): %.o : %.S
	@$(ZZ_GCC_SOURCE) $(CFLAGS) $(HOOKZZ_INCLUDE) -c $< -o $@
	@echo "$(OK_COLOR)generate [$@]! $(NO_COLOR)"

clean:
	@rm -rf $(shell find ./src -name "*\.o" | xargs echo)
	@rm -rf $(OUTPUT_DIR)
	@echo "$(OK_COLOR)clean all *.o success!$(NO_COLOR)"