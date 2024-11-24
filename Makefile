include Kbuild

OBJ_FILE := $(obj-m)
SRC_FILE := $(OBJ_FILE:.o=.c)
CMD_FILE := .$(OBJ_FILE).cmd
MODNAME := $(OBJ_FILE:.o=)
KDIR ?= ${PWD}/../../../orangepi-build/kernel/orange-pi-5.10-rk35xx/
PWD ?= ${PWD}

all:
	make -C ${KDIR} M=${PWD} ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules

clean:
	make -C ${KDIR} M=${PWD} clean
