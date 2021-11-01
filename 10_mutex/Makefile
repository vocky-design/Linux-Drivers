KERNALDIR := /home/vocky/linux/linux-imx-4.1.15-2.1.0-gad512fa-v1.7

CURRENT_PATH := $(shell pwd)

obj-m := 10_mutex.o

build:
	$(MAKE) -C $(KERNALDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNALDIR) M=$(CURRENT_PATH) clean