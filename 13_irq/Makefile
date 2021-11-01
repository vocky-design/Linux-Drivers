KERNALDIR := /home/vocky/linux/linux-imx-4.1.15-2.1.0-gad512fa-v1.7

CURRENT_PATH := $(shell pwd)

obj-m := 13_irq.o

build:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNALDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNALDIR) M=$(CURRENT_PATH) clean