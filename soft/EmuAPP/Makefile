##############################################################################
##
##  Makefile with automatic dependencies generation
##
##  (C) 2008, Alexey Presniakov (a@ooo-klad.ru)
##
##############################################################################



##############################################################################
##  Source files with paths
##############################################################################

ASM_SOURCES+= \
	src/IntVectors.s \


C_SOURCES+= \
	src/startup.c	\
	src/timer0.c	\
	src/i2s.c		\
	src/gpio_lib.c	\
	src/tv.c		\
	src/main.c		\
	src/ps2.c		\
	src/xprintf.c	\
	src/str.c		\
	src/reboot.c	\
	src/crc8.c		\
	src/ffs.c		\
	src/align4.c	\
	src/CPU.c		\
	src/Key.c		\
	src/spi_flash.c \
	src/ui.c		\
	src/ui_key.c	\
	src/menu.c		\
	src/fileman.c	\
	src/ovl.c		\
	src/emu.c		\
	src/tape.c		\


##############################################################################



##############################################################################
##  Directories and file names
##############################################################################
# Output file name
OUT=emu
# Path for source files
SRCDIR=src
# Path for binary files
OUTDIR=out
# Path for object files
OBJDIR=.obj
# Path for dependencies information
DEPDIR=.dep
##############################################################################



##############################################################################
##  Compiler information
##############################################################################
BUILD_NUMBER=$(shell git rev-list --count HEAD)

CC="$(GCCESP8266PATH)\xtensa-lx106-elf-gcc.exe"
INCLUDES=-I$(SRCDIR)
CFLAGS=-O3 -g -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -D__ets__ $(INCLUDES)
LIBS=-lgcc
LD_SCRIPT=ld/eagle.app.v6.ld
LDFLAGS_BUILD_NUMBER=-Xlinker --defsym -Xlinker __BUILD_NUMBER__=$(BUILD_NUMBER)
LDFLAGS=-nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static $(LIBS) $(LDFLAGS_BUILD_NUMBER)
ESPTOOL2=../esptool2/esptool2.exe
##############################################################################



# Target ALL
all: $(OUTDIR)/$(OUT)-0x00000.bin


$(OUTDIR)/$(OUT)-0x00000.bin: $(OUTDIR)/$(OUT)
	@echo "Making $(OUT).bin..."; \
	$(ESPTOOL2) -quiet -bin -boot2 -4096 -qio -40 $^ $@

# Target for linker
$(OUTDIR)/$(OUT): $(subst $(SRCDIR)/,$(OBJDIR)/$(SRCDIR)/,$(C_SOURCES:.c=.o)) $(subst $(SRCDIR)/,$(OBJDIR)/$(SRCDIR)/,$(ASM_SOURCES:.s=.o)) 
	@echo "Linking..."; \
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(OUTDIR)/$(OUT).map $(subst $(SRCDIR)/,$(OBJDIR)/$(SRCDIR)/,$(C_SOURCES:.c=.o)) $(subst $(SRCDIR)/,$(OBJDIR)/$(SRCDIR)/,$(ASM_SOURCES:.s=.o)) -T$(LD_SCRIPT) $(LDFLAGS)

# Target for clean
clean:
	rm -f $(OUTDIR)/$(OUT) $(OUTDIR)/$(OUT).map
	rm -rf $(DEPDIR)
	rm -rf $(OBJDIR)

# Target for distclean
distclean:
	rm -f $(OUTDIR)/$(OUT) $(OUTDIR)/$(OUT)-0x*.bin $(OUTDIR)/$(OUT).map
	rm -rf $(DEPDIR)
	rm -rf $(OBJDIR)


# PHONY
.PHONY: all


# Rule for generation of dependency information
$(DEPDIR)/%.d: %.c
	@set -e; \
	echo "Making dependencies for $*.c"; \
	mkdir -p `dirname "$@"`; \
	mkdir -p `dirname "$(OBJDIR)/$*.o"`; \
	ONAME=`echo "$(OBJDIR)/$*.o" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	DNAME=`echo "$@" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	$(CC) -MM $(CFLAGS) $< \
	| sed "1s/.*:/$$ONAME $$DNAME : /" > $@; \
	[ -s $@ ] || rm -f $@

# Rule for compiling C files
$(OBJDIR)/%.o: %.c
	@echo "Compiling $<"; \
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.s
	@echo "Compiling $<"; \
	$(CC) $(CFLAGS) -c -o $@ $<


# Including dependencies infomation
-include $(subst $(SRCDIR)/,$(DEPDIR)/$(SRCDIR)/,$(C_SOURCES:.c=.d))
