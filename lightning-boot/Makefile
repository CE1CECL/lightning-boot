CROSS_COMPILE ?= arm-linux-uclibc-
CFLAGS =-std=gnu99 -mcpu=arm926ej-s -mlittle-endian -msoft-float -ffreestanding -Os -Wall -pedantic -D_LF1000_BOOTLOADER -DSELF_BOOTSTRAP
#CFLAGS =-std=gnu99 -mcpu=arm926ej-s -mlittle-endian -msoft-float -ffreestanding -Os -Wall -pedantic -D_LF1000_BOOTLOADER
LDFLAGS = -static --architecture armv5te -EL -M -Map lightning-boot.map -e StartUp -T lightning.lds

ifneq ($(KERNELDIR),)
CFLAGS += -I$(KERNELDIR)/include/asm/arch -I$(KERNELDIR)/include/linux
endif

# build NFS booting support for the development boards (but not for the Form
# Factor / final boards as they don't have Ethernet)
ifneq ($(TARGET_MACH),LF_LF1000)
CFLAGS += -DNFS_SUPPORT
endif

# debug printing support
ifneq ($(DEBUG),)
CFLAGS += -DDEBUG
endif

# debug stopwatch support
ifneq ($(DEBUG_STOPWATCH),)
CFLAGS += -DDEBUG_STOPWATCH
endif

# u-boot bootloader support
ifeq ($(UBOOT_SUPPORT),1)
CFLAGS += -DUBOOT_SUPPORT
endif

OBJS = startup.o bootstrap.o nand.o jffs2.o crc32.o string.o gpio.o debug.o \
       timer.o tfs.o adc.o

# Boot Splash Screen support
CFLAGS += -DDISPLAY_SUPPORT -DDISPLAY_BACKLIGHT_PWM
OBJS += display.o dpc.o mlc.o pwm.o

# Configure CPU and bus clocks
CFLAGS += -DCONFIGURE_CLOCKS

# Perform CRC32 checks on kernel image
#CFLAGS += -DCHECK_KERNEL_CRC

LINUX_FILES=include/autoconf.h include/mach-types.h include/dpc_hal.h \
	    include/mlc_hal.h include/pwm_hal.h

all: $(OBJS)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c main.c
	$(CROSS_COMPILE)ld $(LDFLAGS) $(OBJS) main.o -o lightning-boot
	$(CROSS_COMPILE)objcopy -S -I elf32-littlearm -O binary lightning-boot lightning-boot.bin
	$(CROSS_COMPILE)objdump -D -b binary -EL -m armv5te lightning-boot.bin > lightning-boot.dis
	du --apparent-size -b lightning-boot.bin

startup.o: startup.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -Wa,--defsym,StartUp=0x0000 -c startup.S

%.o:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $*.c

install: all
	cp lightning-boot.bin /tftpboot/

clean:
	rm -f *.o *.bin *.dis *.map lightning-boot $(LINUX_FILES)

