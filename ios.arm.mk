#    Copyright 2017 jmpews
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.

HOOKZZ_DIR = $(abspath .)

SRC_SOURCES= $(wildcard src/*.c) $(wildcard src/platforms/backend-posix/thread-posix.c)  $(wildcard src/platforms/backend-darwin/*.c) $(wildcard src/platforms/arch-arm/*.c) $(wildcard src/platforms/backend-arm/*.c)
ZZDEPS_SOURCES = $(wildcard src/zzdeps/darwin/*.c) $(wildcard src/zzdeps/common/*.c) $(wildcard src/zzdeps/posix/*.c) 
ALL_SOURCES = $(SRC_SOURCES) $(ZZDEPS_SOURCES) 

SRC_SOURCES_O = $(patsubst %.c,%.o, $(SRC_SOURCES))
ZZDEPS_SOURCES_O = $(patsubst %.c,%.o, $(ZZDEPS_SOURCES))
ALL_SOURCES_O = $(SRC_SOURCES_O)  $(ZZDEPS_SOURCES_O)

OUTPUT_DIR = $(abspath build/ios-arm)

SELF_SRC_DIR = $(abspath src)
SELF_INCLUDE_DIR = $(abspath include)

# capstone framework
CAPSTONE_INCLUDE = $(abspath deps/capstone/include)
CAPSTONE_LIB_DIR = $(abspath deps/capstone)
CAPSTONE_LIB = capstone.ios.armv7

INCLUDE_DIR = -I$(CAPSTONE_INCLUDE) -I$(SELF_INCLUDE_DIR) -I$(SELF_SRC_DIR)
LIB_DIR = -L$(CAPSTONE_LIB_DIR)
LIBS = -l$(CAPSTONE_LIB)

CFLAGS = -O0 -g
CXXFLAGS = $(CFLAGS) -stdlib=libc++ -std=c++11 -gmodules
LDFLAGS =  $(LIB_DIR) $(LIBS)

# OSX macOS
# http://hanjianwei.com/2013/01/27/abi-compatibility-between-c-plus-plus-11-and-c-plus-plus-98/
ZZ_GXX_BIN = `xcrun --sdk iphoneos --find clang++`
ZZ_GCC_BIN = `xcrun --sdk iphoneos --find clang`
ZZ_SDK = `xcrun --sdk iphoneos --show-sdk-path`
ZZ_GCC=$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) $(INCLUDE_DIR) -arch armv7
ZZ_GXX=$(ZZ_GXX_BIN) -isysroot $(ZZ_SDK) $(CXXFLAGS) $(INCLUDE_DIR) -arch armv7

NO_COLOR=\x1b[0m
OK_COLOR=\x1b[32;01m
ERROR_COLOR=\x1b[31;01m
WARN_COLOR=\x1b[33;01m

# ATTENTION !!!
# 1. simple `ar` can't make a 'static library', need `ar -x` to extract `libcapstone.ios.arm64.a` and then `ar rcs` to pack as `.a`
# 2. must `rm -rf  $(OUTPUT_DIR)/libhookzz.static.a`, very important!!!
ios.arm : $(ALL_SOURCES_O)
	@mkdir -p $(OUTPUT_DIR)
	@rm -rf $(OUTPUT_DIR)/*

	@$(ZZ_GCC) -dynamiclib -install_name @executable_path/Frameworks/libhookzz.dylib $(LDFLAGS) $(ALL_SOURCES_O) -o $(OUTPUT_DIR)/libhookzz.dylib
	@ar -rcs $(OUTPUT_DIR)/libhookzz.static.a $(ALL_SOURCES_O) $(CAPSTONE_LIB_DIR)/lib$(CAPSTONE_LIB).o/*

	@echo "$(OK_COLOR)build success for armv7-ios-hookzz! $(NO_COLOR)"

$(SRC_SOURCES_O): %.o : %.c
	@$(ZZ_GCC) -c $< -o $@
	@echo "$(OK_COLOR)generate [$@]! $(NO_COLOR)"

$(ZZDEPS_SOURCES_O): %.o : %.c
	@$(ZZ_GCC) -c $< -o $@
	@echo "$(OK_COLOR)generate [$@]! $(NO_COLOR)"

# -undefined dynamic_lookup
test : ios.arm
	@$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) -arch armv7 -I$(HOOKZZ_DIR)/include -c tests/arm-ios/test_hook_oc_thumb.m -o tests/arm-ios/test_hook_oc_thumb.o
	@$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) -arch armv7 -dynamiclib -install_name @executable_path/Frameworks/test_hook_oc_thumb.dylib -Wl,-U,_func -framework Foundation -L$(OUTPUT_DIR) -lhookzz.static tests/arm-ios/test_hook_oc_thumb.o -arch armv7 -o $(OUTPUT_DIR)/test_hook_oc_thumb.dylib
	@echo "$(OK_COLOR)build [test_hook_oc_thumb.dylib] success for armv7-ios! $(NO_COLOR)"

	@$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) -arch armv7 -I$(HOOKZZ_DIR)/include -c tests/arm-ios/test_hook_open_arm.c -o tests/arm-ios/test_hook_open_arm.o
	@$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) -arch armv7 -dynamiclib -install_name @executable_path/Frameworks/test_hook_open_arm.dylib -Wl,-U,_func -framework Foundation -L$(OUTPUT_DIR) -lhookzz.static tests/arm-ios/test_hook_open_arm.o -o $(OUTPUT_DIR)/test_hook_open_arm.dylib
	@echo "$(OK_COLOR)build [test_hook_open_arm.dylib] success for armv7-ios! $(NO_COLOR)"

	@$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) -arch armv7 -I$(HOOKZZ_DIR)/include -c tests/arm-ios/test_hook_address_thumb.c -o tests/arm-ios/test_hook_address_thumb.o
	@$(ZZ_GCC_BIN) -isysroot $(ZZ_SDK) $(CFLAGS) -arch armv7 -dynamiclib -install_name @executable_path/Frameworks/test_hook_address_thumb.dylib -Wl,-U,_func -framework Foundation -L$(OUTPUT_DIR) -lhookzz.static tests/arm-ios/test_hook_address_thumb.o -o $(OUTPUT_DIR)/test_hook_address_thumb.dylib
	@echo "$(OK_COLOR)build [test_hook_address_thumb.dylib] success for armv7-ios! $(NO_COLOR)"

	@echo "$(OK_COLOR)build [test] success for armv7-ios-hookzz! $(NO_COLOR)"

clean:
	@rm -rf $(ALL_SOURCES_O)
	@rm -rf $(OUTPUT_DIR)
	@echo "$(OK_COLOR)clean all *.o success!$(NO_COLOR)"