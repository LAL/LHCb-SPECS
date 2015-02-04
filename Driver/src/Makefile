#makefile

# Created on: 18 févr. 2013
#     Author: pciexptest
#LINUXINCLUDE    :=  -Iinclude/linux/autoconf.h
KERNELDIR = /lib/modules/$(shell uname -r)/build    
INCLUDE =-I/lib/modules/$(shell uname -r)/build/include -I/usr/include
MODFLAGS        = -DMODULE  
HOSTCFLAGS   = -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -Wmissing-prototypes
MAKEFLAGS += --include-dir=$(KERNELDIR)
#EXTRA_CFLAGS += -DDEBUG_PCI=1 
#EXTRA_CFLAGS += -DDEBUG_IRQ=1 
#EXTRA_CFLAGS += -DDEBUG_SPECS=1
#EXTRA_CFLAGS += -DPROTECTED_ACCESS
CPPFLAGS        := -D__KERNEL__  -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -Wmissing-prototypes $(LINUXINCLUDE) $(INCLUDE)

obj-m   := AltSpecsV2.o
clean   :
	rm AltSpecsV2.o AltSpecsV2.ko
all : module 
AltSpecsV2.o: AltSpecsV2.c

module :	
	echo "build rep:" $(KERNELDIR) 	
	make -C  $(KERNELDIR)  M=$(PWD)
