ifneq ($(MIUI_PRODUCT), )
ifneq ($(MIUI_KERNEL), )
miui_recovery_out := out/patch_device
miui_recovery_product := $(miui_recovery_out)/$(MIUI_PRODUCT)
miui_recovery_ramdisk := $(miui_recovery_product)/ramdisk-recovery.img
miui_recovery_target := $(miui_recovery_product)/recovery.img
miui_recovery_binary := $(call intermediates-dir-for,EXECUTABLES,recovery)/recovery
miui_recovery_sbin := $(TARGET_ROOT_OUT)/sbin
miui_recovery_resource := src/res
miui_recovery_root := $(miui_recovery_product)/root

miui_recoveryimage_args := \
	--kernel $(MIUI_KERNEL) \
	--ramdisk $(miui_recovery_ramdisk)

ifneq ($(MIUI_KERNEL_CMDLINE),)
	miui_recoveryimage_args += --cmdline "$(MIUI_KERNEL_CMDLINE)"
endif

ifneq ($(MIUI_KERNEL_BASE),)
	miui_recoveryimage_args += --base $(MIUI_KERNEL_BASE)
endif

ifneq ($(MIUI_KERNEL_PAGESIZE),)
	miui_recoveryimage_args += --pagesize $(MIUI_KERNEL_PAGESIZE)
endif
#BUILD_RECOVERY_BEFORE := $(filter $(TARGET_ROOT_OUT)/sbin/%, $(ALL_DEFAULT_INSTALLED_MODULES))
.PHONY: $(MIUI_PRODUCT)
$(MIUI_PRODUCT): miui_recovery_out := $(miui_recovery_out)
$(MIUI_PRODUCT): miui_recovery_product := $(miui_recovery_product)
$(MIUI_PRODUCT): miui_recovery_ramdisk := $(miui_recovery_ramdisk)
$(MIUI_PRODUCT): miui_recovery_target := $(miui_recovery_target)
$(MIUI_PRODUCT): miui_recovery_binary := $(miui_recovery_binary)
$(MIUI_PRODUCT): miui_recovery_sbin := $(miui_recovery_sbin)
$(MIUI_PRODUCT): miui_recovery_resource := $(miui_recovery_resource)
$(MIUI_PRODUCT): miui_recovery_root := $(miui_recovery_root)
$(MIUI_PRODUCT): miui_recoveryimage_args := $(miui_recoveryimage_args)
$(MIUI_PRODUCT): MIUI_PRODUCT := $(MIUI_PRODUCT)
$(MIUI_PRODUCT): MIUI_KERNEL := $(MIUI_KERNEL)
$(MIUI_PRODUCT): MIUI_PRODUCT_ROOT := $(MIUI_PRODUCT_ROOT)
$(MIUI_PRODUCT): MIUI_DEVICE_CONFIG := $(MIUI_DEVICE_CONFIG)
$(MIUI_PRODUCT): $(MKBOOTFS) $(MINIGZIP) \
		$(MKBOOTIMG) \
		recoveryimage
	@echo make $(MIUI_PRODUCT) 
	rm -rf $(miui_recovery_product)
	mkdir -p $(miui_recovery_out)
	mkdir -p $(miui_recovery_product)
	mkdir -p $(miui_recovery_root)
	cp -rf $(MIUI_PRODUCT_ROOT) $(miui_recovery_product)/
	cp -rf $(MIUI_KERNEL) $(miui_recovery_product)/
	cp -rf $(miui_recovery_sbin) $(miui_recovery_root)/
	cp -f $(miui_recovery_binary) $(miui_recovery_root)/sbin/
	cp -rf $(miui_recovery_resource) $(miui_recovery_root)/
ifneq ($(MIUI_DEVICE_CONFIG),)
	-cp -f $(MIUI_DEVICE_CONFIG) $(miui_recovery_root)/res/
endif
	@echo make recovery image $(miui_recovery_target)
	$(MKBOOTFS) $(miui_recovery_root) | $(MINIGZIP) > $(miui_recovery_ramdisk)
	$(MKBOOTIMG) $(miui_recoveryimage_args) --output $(miui_recovery_target)
	$(hide) $(call assert-max-image-size, $(miui_recovery_target), $(BOARD_RECOVERYIMAGE_PARTITION_SIZE), raw)

MIUI_PRODUCT_RELEASE := $(MIUI_PRODUCT)_release
.PHONY:$(MIUI_PRODUCT_RELEASE)
$(MIUI_PRODUCT_RELEASE): miui_recovery_target := $(miui_recovery_target)
$(MIUI_PRODUCT_RELEASE): MIUI_PRODUCT := $(MIUI_PRODUCT)
$(MIUI_PRODUCT_RELEASE):$(MIUI_PRODUCT)
	mkdir -p out
	mkdir -p out/release
	cp $(miui_recovery_target) out/release/recovery_$(MIUI_PRODUCT)_2.0.img
#else for MIUI_KERNEL is NULL
else
$(MIUI_PRODUCT):
	@echo do nothing in make $(MIUI_PRODUCT),because no MIUI_KERNEL
endif #end of MIUI_KERNEL judgement
endif #end of MIUI_PRODUCT judgement


