# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
 
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := sc68

# Thanks to this app's shitty build system/design, there's a lot of .c files that aren't OK to build
# so I copypasted the actually built file names from the autotools build of the program.
LOCAL_SRC_FILES := SC68Plugin.cpp \
    file68/src/error68.c \
    file68/src/file68.c \
    file68/src/gzip68.c \
    file68/src/ice68.c \
    file68/src/init68.c \
    file68/src/msg68.c \
    file68/src/option68.c \
    file68/src/registry68.c \
    file68/src/rsc68.c \
    file68/src/string68.c \
    file68/src/timedb68.c \
    file68/src/uri68.c \
    file68/src/vfs68.c \
    file68/src/vfs68_ao.c \
    file68/src/vfs68_curl.c \
    file68/src/vfs68_fd.c \
    file68/src/vfs68_file.c \
    file68/src/vfs68_mem.c \
    file68/src/vfs68_null.c \
    file68/src/vfs68_z.c \
    libsc68/api68.c \
    libsc68/conf68.c \
    libsc68/emu68/emu68.c \
    libsc68/emu68/error68.c \
    libsc68/emu68/getea68.c \
    libsc68/emu68/inst68.c \
    libsc68/emu68/ioplug68.c \
    libsc68/emu68/line0_68.c \
    libsc68/emu68/line1_68.c \
    libsc68/emu68/line2_68.c \
    libsc68/emu68/line3_68.c \
    libsc68/emu68/line4_68.c \
    libsc68/emu68/line5_68.c \
    libsc68/emu68/line6_68.c \
    libsc68/emu68/line7_68.c \
    libsc68/emu68/line8_68.c \
    libsc68/emu68/line9_68.c \
    libsc68/emu68/lineA_68.c \
    libsc68/emu68/lineB_68.c \
    libsc68/emu68/lineC_68.c \
    libsc68/emu68/lineD_68.c \
    libsc68/emu68/lineE_68.c \
    libsc68/emu68/lineF_68.c \
    libsc68/emu68/mem68.c \
    libsc68/emu68/table68.c \
    libsc68/io68/io68.c \
    libsc68/io68/mfp_io.c \
    libsc68/io68/mfpemul.c \
    libsc68/io68/mw_io.c \
    libsc68/io68/mwemul.c \
    libsc68/io68/paula_io.c \
    libsc68/io68/paulaemul.c \
    libsc68/io68/shifter_io.c \
    libsc68/io68/ym_blep.c \
    libsc68/io68/ym_dump.c \
    libsc68/io68/ym_io.c \
    libsc68/io68/ym_puls.c \
    libsc68/io68/ymemul.c \
    libsc68/libsc68.c \
    libsc68/mixer68.c \
    unice68/unice68_pack.c \
    unice68/unice68_unpack.c \
    unice68/unice68_version.c \
#terminator

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/libsc68 \
	$(LOCAL_PATH)/libsc68/sc68 \
	$(LOCAL_PATH)/libsc68/emu68 \
	$(LOCAL_PATH)/file68 \
	$(LOCAL_PATH)/file68/sc68 \
	$(LOCAL_PATH)/unice68

LOCAL_CFLAGS += -DUSE_UNICE68 -DHAVE_CONFIG_H -DEMU68_EXPORT

LOCAL_LDLIBS := -llog -lz

include $(BUILD_SHARED_LIBRARY)
