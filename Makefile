#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# GRAPHICS is a list of directories containing graphics files
# GFXBUILD is the directory where converted graphics files will be placed
#   If set to $(BUILD), it will statically link in the converted
#   files as if they were data files.
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET			:=	netpass
OUTDIR			:=	out
BUILD			:=	build
MAIN_SOURCE		:=	source
SOURCES			:=	$(MAIN_SOURCE) codegen source/scenes source/hmac_sha256 source/quirc/lib
DATA			:=	data
INCLUDES		:=	include
GRAPHICS		:=	gfx
MUSIC			:=	music
APP_AUTHOR		:=	Sorunome
APP_TITLE		:=	NetPass
APP_DESCRIPTION	:=	NetPass: StreetPass in the modern world!
ROMFS			:=	romfs
GFXBUILD		:=	$(ROMFS)/gfx
MUSICBUILD		:=	$(ROMFS)/music
ifneq ($(DEBUG),1)
    ICON			:=	meta/icon.png
else
	ICON			:=	meta/icon-debug.png
endif
BANNER_AUDIO	:=	meta/banner.ogg
BANNER_IMAGE	:=	meta/banner.cgfx
APP_TITLE_INT	:=	
APP_DESC_INT	:=	-gl "NetPass: StreetPass in der modernen Welt!" \
					-pl "NetPass: StreetPass no mundo moderno!" \
					-il "NetPass: StreetPass nel mondo moderno!" \
					-fl "NetPass: StreetPass dans le monde moderne !" \
					-jl "NetPass：現代世界のすれちがい通信！" \
					-rl "NetPass: StreetPass в современном мире!" \
					-sl "NetPass: ¡StreetPass en el mundo moderno!" \
					-scl "NetPass: 现代世界的瞬缘连接！"

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
include $(TOPDIR)/version.env

# Set Git hash if not in release mode
ifndef RELEASE
GIT_HASH := $(shell git rev-parse --short HEAD)
_VERSION_GIT_SHA_ := $(GIT_HASH)
else
_VERSION_GIT_SHA_ :=
endif

ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations -Wno-format-truncation \
			-ffunction-sections \
			$(ARCH)

# Set DEBUG-related flags
ifeq ($(DEBUG),1)
    CFLAGS += -DDEBUG
    SUFFIX := _debug
    $(info [INFO] Compiling with DEBUG enabled)
else
	SUFFIX :=
endif


CFLAGS	+=	$(INCLUDE) -D__3DS__
CFLAGS	+=	-D_VERSION_MAJOR_=$(NETPASS_VERSION_MAJOR) \
			-D_VERSION_MINOR_=$(NETPASS_VERSION_MINOR) \
			-D_VERSION_MICRO_=$(NETPASS_VERSION_MICRO) \
			-D_VERSION_GIT_SHA_=\"$(_VERSION_GIT_SHA_)\" \
			-D_WELCOME_VERSION_=$(NETPASS_WELCOME_VERSION) \
			-D_PATCHES_VERSION_=$(NETPASS_PATCHES_VERSION) \
			-DNUM_LOCATIONS=$(NETPASS_NUM_LOCATIONS) \
			-I$(DEVKITPRO)/portlibs/3ds/include/opus

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lcitro2d -lcitro3d -lctru -lopusfile -lopus -logg `curl-config --libs` -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB) $(PORTLIBS)

CODEGEN_CFILES = ../codegen/lang_strings.c
CODEGEN_OUTPUTS	= codegen/lang_strings.h codegen/lang_strings.c

# Cppcheck options
CPPCHECK_FLAGS = --enable=warning,unusedFunction,performance,portability,missingInclude --inline-suppr --std=c11
CPPCHECK_FLAGS += --language=c --quiet --suppressions-list=.cppcheck.suppress --template=gcc --check-level=exhaustive

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(OUTDIR)/$(TARGET)$(SUFFIX)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
MUSICFILES	:=	$(foreach dir,$(MUSIC),$(notdir $(wildcard $(dir)/*.*)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

CFILES += $(CODEGEN_CFILES)

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
ifeq ($(GFXBUILD),$(BUILD))
#---------------------------------------------------------------------------------
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

export APP_ICON := $(TOPDIR)/$(BUILD)/icon.png
# If you have python 3 you may have to replase "python" with "python3"
BANNERTOOL	?=	bannertool
FFMPEG		?=	ffmpeg
PYTHON		?=	python

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(OUTPUT).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean translations patches submodulecheck cppcheck

MAKEROM		?=	makerom

MAKEROM_ARGS	:= -elf $(OUTPUT).elf -rsf meta/netpass.rsf -major ${NETPASS_VERSION_MAJOR} -minor ${NETPASS_VERSION_MINOR} -micro ${NETPASS_VERSION_MICRO} -icon $(OUTPUT).smdh -banner "$(BUILD)/banner.bnr"

#---------------------------------------------------------------------------------
3dsx: submodulecheck $(CODEGEN_OUTPUTS) $(BUILD) $(GFXBUILD) $(MUSICBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES) $(OUTDIR) smdh
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

all: build cia

.PHONY: release
release:
	@$(MAKE) RELEASE=1 build
	@$(MAKE) RELEASE=1 cia

$(BUILD):
	@mkdir -p $(BUILD)

patches:
	@$(MAKE) -C patches

$(CODEGEN_OUTPUTS): codegen.py $(shell find locale)
	@$(PYTHON) $(TOPDIR)/codegen.py

smdh: $(APP_ICON)
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" $(APP_TITLE_INT) -l "$(APP_DESCRIPTION)" $(APP_DESC_INT) -p "$(APP_AUTHOR)" -i "$(APP_ICON)" -f visible,allow3d -o $(OUTPUT).smdh

cia: 3dsx
	@$(FFMPEG) -y -i $(TOPDIR)/$(BANNER_AUDIO) -c:a pcm_s16le $(TOPDIR)/$(BUILD)/banner.wav
	@$(BANNERTOOL) makebanner -ci "$(TOPDIR)/$(BANNER_IMAGE)" -a "$(TOPDIR)/$(BUILD)/banner.wav" -o "$(BUILD)/banner.bnr"
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" $(APP_TITLE_INT) -l "$(APP_DESCRIPTION)" $(APP_DESC_INT) -p "$(APP_AUTHOR)" -i "$(APP_ICON)" -f "$(ICON_FLAGS)" -o "$(BUILD)/icon.icn"
	@$(MAKEROM) -f cia -o "$(OUTPUT).cia" -target t -exefslogo $(MAKEROM_ARGS)

$(APP_ICON): $(TOPDIR)/$(ICON)
	@mkdir -p $(BUILD)
	@$(FFMPEG) -y -i $(TOPDIR)/$(ICON) -vf scale=48:48 -frames:v 1 -update 1 $(APP_ICON)

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(MUSICBUILD),$(BUILD))
$(MUSICBUILD):
	@mkdir -p $@
	@for file in $(MUSICFILES) ; do \
		ffmpeg -y -i $(MUSIC)/$$file -ar 48000 -ac 2 $(MUSICBUILD)/$${file%.*}.opus ; \
	done
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

$(OUTDIR):
	@mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@$(MAKE) -C patches clean
	@rm -fr $(BUILD) $(GFXBUILD) $(MUSICBUILD) $(DEPSDIR) $(OUTDIR) $(TOPDIR)/codegen

submodulecheck:
	@test -f source/hmac_sha256/hmac_sha256.h || (echo "ERROR: Submodules not pulled!"; exit 1)

cppcheck:
	cppcheck $(CPPCHECK_FLAGS) $(MAIN_SOURCE)

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x	$(BUILD)/%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x
#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x %.shbin
#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
%.shbin.o %_shbin.h : %.shbin
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
