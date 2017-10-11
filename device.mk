#EG_SRC = $(shell pwd)
EG_SRC = project/mt7687_hdk/apps/eg_device

C_FILES += $(EG_SRC)/src/device.c
C_FILES += $(EG_SRC)/src/ccd_device*.c
C_FILES += $(EG_SRC)/src/platform_os*.c
C_FILES += $(EG_SRC)/src/device_hal*.c
#CFLAGS += -I$(EG_SRC)/inc
CFLAGS  += -I$(SOURCE_DIR)/project/mt7687_hdk/apps/eg_device/inc


