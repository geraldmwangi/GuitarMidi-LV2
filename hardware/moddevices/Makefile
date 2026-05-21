PREFIX ?= /usr
LV2DIR ?= $(PREFIX)/lib/lv2

PLUGIN = guitarmidi
BUNDLE = $(PLUGIN).lv2

INSTALL_DIR = $(DESTDIR)$(LV2DIR)
BUILD_DIR = build
# Use absolute path for SOURCE_DIR to avoid pathing issues in sub-cmake calls
SOURCE_DIR = $(shell pwd)/../../
EXT_DIR = $(SOURCE_DIR)/ext

# Use -march=armv8-a to strictly stick to MOD Dwarf (RK3399) capabilities
ARMCC_FLAGS="-funsafe-math-optimizations -march=armv8-a -mcpu=generic"

# this flatbuffers version must be installed on the host system
$(PLUGIN).so: 
	echo "Source dir $(SOURCE_DIR). Dest dir $(DESTDIR). Install dir $(INSTALL_DIR)"
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DBUILD_DEB=OFF \
        -DCMAKE_C_FLAGS=$(ARMCC_FLAGS) \
        -DCMAKE_CXX_FLAGS=$(ARMCC_FLAGS) \
        -DCMAKE_ASM_FLAGS=$(ARMCC_FLAGS) \
        -DTFLITE_HOST_TOOLS_DIR="/usr/bin" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
        -DFETCHCONTENT_SOURCE_DIR_FFT2D=$(EXT_DIR)/fft2d/OouraFFT-1.0 \
        -DFETCHCONTENT_SOURCE_DIR_NEON2SSE=$(EXT_DIR)/neon2sse/ARM_NEON_2_x86_SSE-a15b489e1222b2087007546b4912e21293ea86ff \
        -DPTHREADPOOL_SOURCE_DIR=$(EXT_DIR)/pthreadpool/pthreadpool-c2ba5c50bb58d1397b693740cf75fad836a0d1bf \
        -DFP16_SOURCE_DIR=$(EXT_DIR)/fp16/FP16-0a92994d729ff76a58f692d3028ca1b64b145d91 \
        -DFXDIV_SOURCE_DIR=$(EXT_DIR)/fxdiv/FXdiv-b408327ac2a15ec3e43352421954f5b1967701d1 \
        -DFETCHCONTENT_SOURCE_DIR_FLATBUFFERS=$(EXT_DIR)/flatbuffers/flatbuffers-24.3.25 \
	-Dflatbuffers_SOURCE_DIR=$(EXT_DIR)/flatbuffers/flatbuffers-24.3.25 \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
	-DXNNPACK_TARGET_PROCESSOR:STRING=generic \
        -DXNNPACK_BUILD_ALL_MICROKERNELS:BOOL=OFF \
        -DXNNPACK_BUILD_PROD_MICROKERNELS:BOOL=OFF \
        -DXNNPACK_ENABLE_ASSEMBLY:BOOL=OFF \
        -DXNNPACK_ENABLE_ARM_NEON:BOOL=ON \
        -DTFLITE_ENABLE_XNNPACK:BOOL=ON \
        -DTFLITE_ENABLE_KLEIDIAI:BOOL=OFF \
        -DXNNPACK_ENABLE_KLEIDIAI:BOOL=OFF \
        -DXNNPACK_BUILD_KLEIDIAI:BOOL=OFF \
        -DXNNPACK_ENABLE_ARM_I8MM:BOOL=OFF \
        -DXNNPACK_ENABLE_ARM_DOTPROD:BOOL=OFF \
        -DXNNPACK_ENABLE_ARM_BF16:BOOL=OFF \
        -DXNNPACK_ENABLE_ARM_FP16_VECTOR:BOOL=OFF \
        -DXNNPACK_ENABLE_ARM_FP16_SCALAR:BOOL=OFF \
        -DXNN_ENABLE_ARM_I8MM:BOOL=OFF \
        -DXNN_ENABLE_ARM_DOTPROD:BOOL=OFF \
        -DXNN_ENABLE_ARM_BF16:BOOL=OFF \
        -DXNN_ENABLE_ARM_FP16_VECTOR:BOOL=OFF \
        -DXNN_ENABLE_ARM_FP16_SCALAR:BOOL=OFF \
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
        $(SOURCE_DIR)
	cd $(BUILD_DIR) && cmake  --build . -j 15

install:
	cd $(BUILD_DIR) && cmake --install . --prefix /usr/lib/lv2 #$(INSTALL_DIR) 

clean:
	rm -rf $(BUILD_DIR)

