############same as android###############
#make $(MIUI_PRODUCT) to buid device recovery.img, support as following;
MIUI_PRODUCTS := crespo ville sensation vivo saga maguro honor shooteru \
	lu6200  p1 d1 finder note2
#crespo: Samsung Nexus S
#ville: HTC One S
#sensation: HTC pyramid(G14)
#vivo: HTC Incredible S
#saga: HTC Desire S
#maguro: Samsung Glaxy Nexus
#honor: HUAWEI honor
#shooteru: HTC EVDO 3D (G17)
#lu6200: LG lu6200
#p1:HUAWEI U9200
#d1:HUAWEI U9500
#finder: opper finder 4
#note2: SAMSUNG Glaxy Note 2
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
.PHONY:release
release:
	make clean;
	make;
	$(foreach product, $(MIUI_PRODUCTS),make $(product)_release;)

