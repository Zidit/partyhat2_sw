
#Port for programing
PROG_PORT = /dev/ttyUSB0

TARGETNAME = flash

SRC = \
cr_startup_lpc82x.c \
crp.c \
editor.c \
main.c \
mtb.c \
nvm.c \
printf.c \
ring_buffer.c \
serial.c \
sine.c \
sysinit.c \
ws2812.c \
ubasic/tokenizer.c \
ubasic/ubasic.c \
lpcopen/src/acmp_8xx.c \
lpcopen/src/adc_8xx.c \
lpcopen/src/chip_8xx.c \
lpcopen/src/clock_8xx.c \
lpcopen/src/crc_8xx.c \
lpcopen/src/dma_8xx.c \
lpcopen/src/gpio_8xx.c \
lpcopen/src/i2c_common_8xx.c \
lpcopen/src/i2cm_8xx.c \
lpcopen/src/i2cs_8xx.c \
lpcopen/src/iap.c \
lpcopen/src/iocon_8xx.c \
lpcopen/src/irc_8xx.c \
lpcopen/src/pinint_8xx.c \
lpcopen/src/pmu_8xx.c \
lpcopen/src/ring_buffer.c \
lpcopen/src/sct_8xx.c \
lpcopen/src/sct_pwm_8xx.c \
lpcopen/src/spi_8xx.c \
lpcopen/src/spim_8xx.c \
lpcopen/src/spis_8xx.c \
lpcopen/src/stopwatch_8xx.c \
lpcopen/src/swm_8xx.c \
lpcopen/src/syscon_8xx.c \
lpcopen/src/sysinit_8xx.c \
lpcopen/src/uart_8xx.c \
lpcopen/src/wkt_8xx.c \
lpcopen/src/wwdt_8xx.c


LDLIBS = 

# Source dir name, include directories, and sub source dir
SRCDIR = ./src
BUILDDIR = ./build
OBJDIR = $(BUILDDIR)/obj
DEPDIR = $(BUILDDIR)/deb
ASMDIR = $(BUILDDIR)/asm

BASEDIR = ./
INCDIR = $(BASEDIR)/src/include $(BASEDIR)/src/lpcopen/inc $(BASEDIR)/src/ubasic. /usr/local/include
LIBDIR = $(BASEDIR)

# Flags for above programs when calling them from the command line

CFLAGS = -DDEBUG -D__CODE_RED -DCORE_M0PLUS -D__USE_ROMDIVIDE -D__USE_LPCOPEN -DNO_BOARD_LIB -D__LPC82X__ -g3 -mcpu=cortex-m0plus -march=armv6-m -mthumb -Og 
LDFLAGS = -Xlinker -Map="$(BUILDDIR)/Partyhat.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m0plus -march=armv6-m  -T "configurations.ld"
LDLIBS := -Wl,-lgcc $(addprefix -l,$(LDLIBS))

# Programs to use for creating dependencies, compiling source files, and creating the library file, respectively
CC  = arm-none-eabi-gcc
ECHO = @echo
RM = rm
MKDIR = mkdir
CAT = @cat


# Name of library file, list of source files, object files, and dependency files
ELFFILE  = $(BUILDDIR)/$(TARGETNAME).elf
OBJFILES = $(addprefix $(OBJDIR)/,$(subst .c,.o,$(SRC)))
DEPFILES = $(addprefix $(DEPDIR)/,$(subst .c,.d,$(SRC)))
ASMFILES = $(addprefix $(ASMDIR)/,$(subst .c,.s,$(SRC)))

#Combine all flags for linking, building an creatin depency files
LDFLAGS_ALL = $(OBJFILES) $(LDFLAGS) -L $(LIBDIR) $(LDLIBS)
CFLAGS_ALL = $(CFLAGS) -MMD -MP -c $(addprefix -I,$(INCDIR))


all: build

build: build_begin $(ELFFILE) build_end
asm: build_begin asm_build build_end

build_begin:
	$(ECHO)
	$(ECHO) "---------------Build begin---------------"
	$(ECHO) "Building target $(ELFFILE)"
	@$(CC) --version
	$(ECHO)
	$(ECHO) "Compiler flags: $(CFLAGS)"
	$(ECHO) "Linker flags:   $(LDFLAGS)"
	$(ECHO) "Libraries:      $(LDLIBS)"
	$(ECHO) "-----------------------------------------"
	$(ECHO)

build_end:
	$(ECHO)
	$(ECHO) "----------------Build end----------------"
	$(ECHO)


#Build c files to objects
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(ECHO)
	$(ECHO) "Building object file '$@'"
	@mkdir -p $(DEPDIR)/$(*D)
	@mkdir -p $(@D)
	$(CC) "$<" -o "$@" -MF "$(DEPDIR)/$(*F).d" $(CFLAGS_ALL)


#Link objects
$(ELFFILE): $(OBJFILES)
	$(ECHO)
	$(ECHO) "Linking $(TARGETNAME).elf"
	$(CC) -o $(ELFFILE) $(LDFLAGS_ALL)
	$(ECHO)

asm_build: $(ASMFILES)

$(ASMDIR)/%.s: $(SRCDIR)/%.c
	$(ECHO)
	$(ECHO) "Building asm file '$@'"
	@mkdir -p $(ASMDIR)/$(*D)
	@mkdir -p $(@D)
	$(CC) "$<" -o "$@" $(CFLAGS_ALL) -S

#Delete all files and folders in BUILDDIR
clean:
	$(if $(BUILDDIR),$(RM) -rf $(BUILDDIR)/*)

bin: build 
#$(ELFFILE)
	arm-none-eabi-objcopy -O binary $(ELFFILE) $(BUILDDIR)/$(TARGETNAME).bin

hex: build
#$(ELFFILE)
	arm-none-eabi-objcopy -O ihex $(ELFFILE) $(BUILDDIR)/$(TARGETNAME).hex

program: bin
	$(ECHO)
	$(ECHO) "---------------Programing----------------"
	$(ECHO) 
	python lpcprog.py -p $(PROG_PORT) $(BUILDDIR)/$(TARGETNAME).bin


#Include depency files created earlier
-include $(DEPFILES)


#.PHONY: build all

.SECONDARY: $(OBJFILES) $(DEPFILES)
.DEFAULT: all

