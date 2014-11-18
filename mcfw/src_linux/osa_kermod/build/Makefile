
ifneq ($(KERNELRELEASE),)
# call from kernel build system

EXTRA_CFLAGS = -I$(M)/../inc -I$(M)/../priv -I$(M)/../../osa/inc -DKERNEL_LEVEL -DDEBUG -O3
                                                          
osa_kermod-objs := ../src/dev_drv.o ../src/dev_dma.o ../src/dev_dmaCopyFill.o            
obj-m	:= osa_kermod.o

else

PWD     := `pwd`
SUBDIRS := build src 
KER_MODULE := osa_kermod

modules:
	@echo \# $(KER_MODULE): Building module
ifneq ($(OS),Windows_NT)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi- modules
else
	@echo \# Bypassing ... $(KER_MODULE): Building module for windows
endif
  
install:
	@echo \# $(KER_MODULE): Copying files to install directory
	-mkdir -p $(TARGET_FS_DIR)/kermod
	-mkdir -p $(TARGET_FS_DIR)/scripts	
ifneq ($(OS),Windows_NT)
	cp osa_kermod.ko $(TARGET_FS_DIR)/kermod
else
	@echo \# Bypassing cp osa_kermod.ko $(TARGET_FS_DIR)/kermod
endif
	cp osa_kermod*.sh $(TARGET_FS_DIR)/scripts
	
clean:
	@echo \# $(KER_MODULE): Deleting temporary files
	for i in $(SUBDIRS) ; do cd ../$$i; rm -rf *.ko *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.order; cd -; done  
	
depend:
	@echo \# $(KER_MODULE): Dependencies..
endif



