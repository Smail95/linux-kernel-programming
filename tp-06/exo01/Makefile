KERNELDIR_PNL ?= /home/aider/vm/linux-4.19
PWD := $(shell pwd)

obj-m +=  hellosysfs.o

all :
	make -C $(KERNELDIR_PNL) M=$(PWD) modules
 
clean:
	make -C $(KERNELDIR_PNL) M=$(PWD) clean 
