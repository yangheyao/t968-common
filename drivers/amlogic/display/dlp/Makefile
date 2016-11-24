#obj-$(CONFIG_YZS_DLP) += dlp3438.o
#obj-$(CONFIG_YZS_POWER_DLP) += power_dlp3438.o
#obj-$(CONFIG_AML_DLP_POWER) += power_dlp_gpio.o
#obj-$(CONFIG_AML_DLP) += power_dlp_test.o

ifneq ($(KERNELRELEASE),)
	obj-m := m_gpio.o
	m_gpio-y:= power_dlp_test.o

else
	PWD=$(shell pwd)
	KVER=$(shell uname -r)
	KDIR=/lib/modules/$(KVER)/build
all:
	make -C $(KDIR) M=$(PWD)
clean:
	rm *.o *.ko modules.* Module.symvers *.mod.c
endif
