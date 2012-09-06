############same as android###############
MIUI_PRODUCTS := crespo maguro honor ones vivo saga 
MIUI_PRODUCTS := $(strip $(MIUI_PRODUCTS))

.PHONY: default
default: 
	make recovery -B -j32 
	@echo make default

.PHONY: usage
usage:
	@echo "env_setup.sh must be executed at the beginning"
	@echo "$(MIUI_PRODUCTS) has support to make recoveryimage"
	@echo "    such as make crespo, make maguro or etc"
	@echo "make [target] -j[n] for making targe"
	@echo "make [target] -B for reset to make target"
	@echo "make clean for clean every everything that build out"
	@echo "make device_clean for clean the device recovery"

##include main.mk ,load make system#######
include build/core/main.mk
##########################################
