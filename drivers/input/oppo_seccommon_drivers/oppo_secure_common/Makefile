# only 6765
DEFS_PLATFORM = -DCONFIG_OPPO_BSP_SECCOM_PLATFORM=6765
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_export_inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/module

ifneq ($(TARGET_PRODUCT),qssi)
ccflags-y += $(DEFS_PLATFORM)
obj-y     +=  oppo_secure_common.o
endif

