
obj-m	:= ti81xxfb.o
ti81xxfb-y := ti816x_fb_main.o ti816x_fb_ioctl.o

PWD     := `pwd`
KDIR	:= $(KERNELDIR) 

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) ARCH=arm CROSS_COMPILE=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi- modules
	chmod 775 ti81xxfb.ko
	-mkdir -p $(TARGET_FS_DIR)/kermod
#	cp $(dvr_rdk_PATH)/mcfw/src_linux/kernel_modules/ti816x_fb/ti81xxfb.ko $(TARGET_FS_DIR)/kermod/.

depend:
# nothing to be done for depend for .ko

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
