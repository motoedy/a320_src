# Automatically generated by configure
# Configured with: './configure' '--platform=gcw0'
CC = mipsel-linux-gcc
CXX = mipsel-linux-g++
AS = mipsel-linux-as
CFLAGS +=  -march=mips32 -DUSE_JZ47XX -DRAM_FIXED -Wno-unused-result
ASFLAGS +=  -marcg=mips32 -DUSE_JZ47XX
LDFLAGS += 
MAIN_LDFLAGS += 
MAIN_LDLIBS += -lasound -lpng  -ldl -lm -lpthread -lz
PLUGIN_CFLAGS +=  -fPIC

ARCH = mips
PLATFORM = gcw0
BUILTIN_GPU = peops
SOUND_DRIVERS = oss alsa
PLUGINS = plugins/spunull/spunull.so plugins/dfxvideo/gpu_peops.so plugins/gpu_unai/gpu_unai.so
USE_DYNAREC = 1
DRC_CACHE_BASE = 1
