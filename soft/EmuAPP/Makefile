##############################################################################
##  Source files with paths
##############################################################################

##############################################################################
##  Source files with paths
##############################################################################

FAST_C_SOURCES += \
	src/AnyMem.c					\
	src/CPU.c						\
	src/Key.c						\
	src/ovl.c						\
	src/ps2.c						\
	src/spi_flash.c					\
	src/tv.c						\
	src/i2s.c						\
	src/Overlays/Emu/CPU_e.c		\
	src/Overlays/Emu/emu_e.c		\
	src/Overlays/EmuFfs/CPU_ef.c	\
	src/Overlays/EmuFfs/emu_ef.c	\
	src/Overlays/EmuUi/Key_eu.c		\
	src/Overlays/EmuUi/ps2_eu.c		\

C_SOURCES += \
	src/Debug.c						\
	src/ffs.c						\
	src/reboot.c					\
	src/Startup.c					\
	src/Overlays/Ffs/ffs_f.c		\
	src/Overlays/Ffs/spi_flash_f.c	\
	src/Overlays/Ffs/tape_f.c		\
	src/Overlays/FfsUi/crc8_fu.c	\
	src/Overlays/FfsUi/ffs_fu.c		\
	src/Overlays/FfsUi/str_fu.c		\
	src/Overlays/Init/CPU_i.c		\
	src/Overlays/Init/crc8_i.c		\
	src/Overlays/Init/ffs_i.c		\
	src/Overlays/Init/i2s_i.c		\
	src/Overlays/Init/main_i.c		\
	src/Overlays/Init/ps2_i.c		\
	src/Overlays/Init/spi_flash_i.c	\
	src/Overlays/Init/tv_i.c		\
	src/Overlays/Ui/fileman_u.c		\
	src/Overlays/Ui/menu_u.c		\
	src/Overlays/Ui/str_u.c			\
	src/Overlays/Ui/ui_u.c			\
	src/Overlays/Ui/xprintf_u.c		\

INCLUDES += src

#CPP_SOURCES= \
#    ../Src/main.cpp

ASM8_SOURCES += \
	src/IntVectors.as8	\
	src/Startup.as8	\


#ASM_SOURCES += \
#	src/IntVectors.s	\

##############################################################################
##  Directories and file names
##############################################################################
# Output file name
OUT                =emu
# Path for binary files
OUTDIR             =out
# Path for object files
OBJDIR             =.obj
# Path for dependencies information
DEPDIR             =.dep

#LIBS=

BUILD_NUMBER       = $(shell git rev-list --count HEAD)

# Linker script file
LDFLAGS           += -Tld/eagle.app.v6.ld

#FAST_GCC_FLAGS   += -fmerge-all-constants -ffunction-sections -Wshadow -fomit-frame-pointer -fno-strict-aliasing -fno-common -fno-builtin -fno-exceptions
FAST_GCC_FLAGS    += -O3     -nostdlib -fno-inline-functions -Wall $(addprefix -I,$(INCLUDES)) -g
# Xtensa options
FAST_GCC_FLAGS    += -mlongcalls -mtext-section-literals
# defines
FAST_GCC_FLAGS    += -D__ets__

COMMAN_GCC_FLAGS  += -O3 -Os -nostdlib -fno-inline-functions -Wall $(addprefix -I,$(INCLUDES)) -g
# Xtensa options
COMMAN_GCC_FLAGS  += -mlongcalls -mtext-section-literals
# defines
COMMAN_GCC_FLAGS  += -D__ets__

LDFLAGS           += -Wl,-EL -Wl,--no-check-sections -u call_user_start -Wl,-static $(LIBS) -Xlinker --defsym -Xlinker __BUILD_NUMBER__=$(BUILD_NUMBER) -Wl,-Map,$(OUTDIR)/$(OUT).map
#LDFLAGS          += -Wl,--gc-sections

CFLAGS            += -x c
CPPFLAGS          += -x c++
ASMFLAGS          += -x assembler-with-cpp


#include $(PRJROOT)/MakeFiles/V1.1.0/Common_Bin_Files.mk
ASMPREP     =../ask2asm/ask2asm.exe
#include $(PRJROOT)/MakeFiles/V1.1.0/GCC_ARM_Common_Bin_Files.mk
CC			="$(GCCESP8266PATH)\xtensa-lx106-elf-gcc.exe"
OBJCOPY		="$(GCCESP8266PATH)\xtensa-lx106-elf-objcopy.exe"
OBJDUMP		="$(GCCESP8266PATH)\xtensa-lx106-elf-objdump.exe"
ESPTOOL2    =../esptool2/esptool2.exe

#include $(PRJROOT)/MakeFiles/V1.1.0/GCC_ARM_Common_Options.mk

#include $(PRJROOT)/MakeFiles/V1.1.0/ARM_Common_Link_Src.mk
LINKSRC += \
 $(addprefix $(OBJDIR)/, \
 $(addsuffix .c.o,        $(C_SOURCES)) \
 $(addsuffix .fc.o,       $(FAST_C_SOURCES)) \
 $(addsuffix .cpp.o,      $(CPP_SOURCES)) \
 $(addsuffix .s.o,        $(ASM_SOURCES)) \
 $(addsuffix .as8.o,      $(ASM8_SOURCES)))


#include $(PRJROOT)/MakeFiles/V1.1.0/ARM_Common_Make_static_mmu_cfg_uboot_image.mk
# Target ALL
all: $(OUTDIR)/$(OUT)-0x00000.bin

# PHONY
.PHONY: all

$(OUTDIR)/$(OUT)-0x00000.bin: $(OUTDIR)/$(OUT).elf
	@echo "Making $(OUT).bin..."; \
	$(ESPTOOL2) -quiet -bin -boot2 -4096 -qio -40 $^ $@

#include $(PRJROOT)/MakeFiles/V1.1.0/Common_Clean_Rules.mk
# Target for clean
clean:
	@rm -rf $(OBJDIR)/*; \
	rm  -rf $(DEPDIR)/*; \
	rm -rf $(OUTDIR)/$(OUT).map; \
	rm -rf $(OUTDIR)/$(OUT).log

# Target for distclean
distclean:
	@rm -rf $(OUTDIR)/$(OUT)*; \
	rm -rf $(OBJDIR); \
	rm -rf $(DEPDIR)

# Target for linker
$(OUTDIR)/$(OUT).elf: $(LINKSRC)
	@echo "Linking..."; \
	mkdir -p $(OUTDIR); \
	$(CC) $(COMMAN_GCC_FLAGS) $(LDFLAGS) -o $(OUTDIR)/$(OUT).elf $(LINKSRC)

disassemble: $(addprefix $(OBJDIR)/, \
	$(addsuffix .c.lst,        $(C_SOURCES)) \
	$(addsuffix .fc.lst,       $(FAST_C_SOURCES)) \
	$(addsuffix .cpp.lst,      $(CPP_SOURCES)) \
	$(addsuffix .s.lst,        $(ASM_SOURCES)) \
	$(addsuffix .as8.lst,      $(ASM8_SOURCES)))

%.lst: %.o
	@echo "Disassembling for $<"; \
	mkdir -p $(dir $@); \
	$(OBJDUMP) -d -S $< >$@; \
	[ -s $@ ] || rm -f $@

# Rule for generation of dependency information
$(DEPDIR)/%.c.d: %
	@set -e; \
	echo "Making dependencies for $<"; \
	mkdir -p $(dir $@); \
	mkdir -p $(dir $(OBJDIR)/$*.c.o); \
	ONAME=`echo "$(OBJDIR)/$*.c.o" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	DNAME=`echo "$@" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	$(CC) -MM $(COMMAN_GCC_FLAGS) $(CFLAGS) $< | sed "1s/.*:/$$ONAME $$DNAME : /" > $@; \
	[ -s $@ ] || rm -f $@

# Rule for compiling C files
$(OBJDIR)/%.c.o: %
	@echo "Compiling $<"; \
	$(CC) $(COMMAN_GCC_FLAGS) $(CFLAGS) -c -o $@ $<

# Rule for compiling C files
$(OBJDIR)/%.c.s: %
	@echo "Compiling $<"; \
	$(CC) $(COMMAN_GCC_FLAGS) $(CFLAGS) -S -o $@ $<

# Rule for generation of dependency information
$(DEPDIR)/%.fc.d: %
	@set -e; \
	echo "Making dependencies for $<"; \
	mkdir -p $(dir $@); \
	mkdir -p $(dir $(OBJDIR)/$*.fc.o); \
	ONAME=`echo "$(OBJDIR)/$*.fc.o" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	DNAME=`echo "$@" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	$(CC) -MM $(FAST_GCC_FLAGS) $(CFLAGS) $< | sed "1s/.*:/$$ONAME $$DNAME : /" > $@; \
	[ -s $@ ] || rm -f $@

# Rule for compiling C files
$(OBJDIR)/%.fc.o: %
	@echo "Compiling $<"; \
	$(CC) $(FAST_GCC_FLAGS) $(CFLAGS) -c -o $@ $<

# Rule for compiling C files
$(OBJDIR)/%.fc.s: %
	@echo "Compiling $<"; \
	$(CC) $(FAST_GCC_FLAGS) $(CFLAGS) -S -o $@ $<

$(DEPDIR)/%.cpp.d: %
	@set -e; \
	echo "Making dependencies for $<"; \
	mkdir -p $(dir $@); \
	mkdir -p $(dir $(OBJDIR)/$*.cpp.o); \
	ONAME=`echo "$(OBJDIR)/$*.cpp.o" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	DNAME=`echo "$@" | sed -e 's/\\//\\\\\\//g' | sed -e 's/\\./\\\\\\./g'`; \
	$(CC) -MM $(COMMAN_GCC_FLAGS) $(CPPFLAGS) $< | sed "1s/.*:/$$ONAME $$DNAME : /" > $@; \
	[ -s $@ ] || rm -f $@

# Rule for compiling C files
$(OBJDIR)/%.cpp.o: %
	@echo "Compiling $<"; \
	$(CC) $(COMMAN_GCC_FLAGS) $(CPPFLAGS) -c -o $@ $<

# Rule for compiling Asm files
$(OBJDIR)/%.s.o: %
	@echo "Assembling $<"; \
	mkdir -p $(dir $@); \
	$(CC) $(COMMAN_GCC_FLAGS) $(ASMFLAGS) -c -o $@ $<

# Rule for compiling Asm8 files
$(OBJDIR)/%.as8.o: %
	@set -e; \
	echo "Preprocessing and assembling $<"; \
	mkdir -p $(dir $@); \
	$(ASMPREP) 2 1 $< $(OBJDIR)/$*.as8.s; \
	$(CC) $(COMMAN_GCC_FLAGS) $(ASMFLAGS) -c -o $@ $(OBJDIR)/$*.as8.s

#include $(PRJROOT)/MakeFiles/V1.1.0/ARM_Common_Include_Dep.mk
# Including dependencies infomation
-include $(addprefix $(DEPDIR)/, \
	$(addsuffix .c.d,        $(C_SOURCES)) \
	$(addsuffix .fc.d,       $(FAST_C_SOURCES)) \
	$(addsuffix .cpp.d,      $(CPP_SOURCES)) \
	$(addsuffix .s.d,        $(ASM_SOURCES)) \
	$(addsuffix .as8.d,      $(ASM8_SOURCES)))
