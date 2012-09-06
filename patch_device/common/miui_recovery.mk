ifneq ($(MIUI_PRODUCT), )
ifneq ($(MIUI_KERNEL), )
miui_recovery_out := $(ANDROID_BUILD_TOP)/out/third_device
miui_recovery_product := $(miui_recovery_out)/$(MIUI_PRODUCT)
miui_recovery_ramdisk := $(miui_recovery_product)/ramdisk-recovery.img
miui_recovery_target := $(miui_recovery_product)/recovery.img
miui_recovery_binary := $(TARGET_RECOVERY_ROOT_OUT)/sbin
miui_recovery_resource := $(ANDROID_BUILD_TOP)/src/res
miui_recovery_root := $(miui_recovery_product)/root
#miui_recovery_device_conf := $(MIUI_DEVICE_CONFIG)

miui_recoveryimage_args := \
	--kernel $(MIUI_KERNEL) \
	--ramdisk $(miui_recovery_ramdisk)

ifdef MIUI_KERNEL
	miui_recoveryimage_args += --cmdline "$(MIUI_KERNEL_CMDLINE)"
endif

ifdef MIUI_KERNEL_BASE
	miui_recoveryimage_args += --base $(MIUI_KERNEL_BASE)
endif

ifdef MIUI_KERNEL_PAGESIZE
	miui_recoveryimage_args += --base $(MIUI_KERNEL_PAGESIZE)
endif

$(MIUI_PRODUCT): $(MKBOOTFS) $(MINIGZIP) \
		$(MKBOOTIMG) recovery
	mkdir -p $(miui_recovery_out)
	mkdir -p $(miui_recovery_product)
	mkdir -p $(miui_recovery_root)
	cp -rf $(MIUI_PRODUCT_ROOT) $(miui_recovery_product)/
	cp -rf $(MIUI_KERNEL) $(miui_recovery_product)/
	-rm -rf $(miui_recovery_root)/sbin
	cp -f $(miui_recovery_binary) $(miui_recovery_root)/
	cp -rf $(miui_recovery_resource) $(miui_recovery_root)/
	-cp -f $(MIUI_DEVICE_CONFIG) $(miui_recovery_root)/res/
	@echo make recovery image $(miui_recovery_target)
	$(MKBOOTFS) $(MIUI_PRODUCT_ROOT) | $(MINIGZIP) > $(miui_recovery_ramdisk)
	$(MKBOOTIMG) $(miui_recoveryimage_args) --output $(miui_recovery_target)
	$(hide) $(call assert-max-image-size, $(miui_recovery_target), $(BOARD_RECOVERYIMAGE_PARTITION_SIZE), raw)
#else for MIUI_KERNEL is NULL
else
$(MIUI_PRODUCT):
	@echo do nothing in make $(MIUI_PRODUCT),because no MIUI_KERNEL
endif #end of MIUI_KERNEL judgement
endif #end of MIUI_PRODUCT judgement

