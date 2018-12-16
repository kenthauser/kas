#
# Copyright (c) 1992 Twenty-First Designs, Inc.
# Unpublished work. All rights reserved.  
#
# THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
# TWENTY-FIRST DESIGNS, INC.
#
# This software source code contains trade secrets and other
# proprietary information of Twenty-First Designs, Inc. All
# disclosure or use of this source code is strictly restricted
# by contract to that required for operation and maintenance 
# of equipment provided by Twenty-First Designs, Inc.
# All other disclosure or use of this source code is prohibited.
#
#**********************************************************
#
# $Id: smn_prog.mk,v 1.4 2001/04/24 05:33:35 kent Exp $
#
#**********************************************************
#
# Makefile common input for all Z-80 programs
#
# Included in all Makefiles.
# 
# make variable `ZROOT' must be defined as directory
# just above the `zos' directory.
#
# make variable `DIR' defaults to `.' Is path to local source
# directory.
#
#**********************************************************

.KEEP_STATE:

# declare Z80 rules
.SUFFIXES: .z80 .hex

DIR = .
ZOS = $(ZROOT)/zos

# do all non-local includes relative DIR
GZ	= z80-aout-gcc -c -cl $(CFLAGS)
#GZ	= i386-aout-gcc -v
#LD	= i386-aout-ld -N -T $(ZOS)/ld_rom.link
LD	= ldz -N -T 0 -Tdata 8000
RANLIB	= i386-aout-ranlib
#SIZE 	= i386-aout-size
SIZE	= size
AR 	= i386-aout-ar

#GZ = gz -target z80 -cl $(CFLAGS)
UC_PGM = echo ${PGM} | tr a-z A-Z

CFLAGS += -D${UC_PGM:sh} -I$(DIR) -I$(ZROOT)
#CFLAGS += -I$(DIR) -I$(ZROOT)

# compilation rules for local source

%.o:$(DIR)/%.z80; $(GZ) $<
%.lst:$(DIR)/%.z80; $(GZ) -cl $<
%.o:%.s; $(GZ) $(CFLAGS) $<

%.hex: %; ihex -o $@ $<
##%.hex: %; i386-aout-objcopy -O ihex -S $< $@

# create default obj list
OBJS += $(SRCS:.z80=.o)
LISTINGS = $(OBJS:.o=.lst)

# declare bootstrap 
%.o:$(ZROOT)/zos/%.z80; $(GZ) $<
BOOT.z80 = $(ZROOT)/zos/boot.z80
BOOT.o = $(BOOT.z80:$(ZROOT)/zos/%.z80=%.o)
LISTINGS += $(BOOT.o:%.o=%.lst)

# assemble & link program
$(PGM): $(BOOT.o) $$(OBJS) $$(LIBS)
	@$(RM) $@ $@.hex
	$(LD) $(LDFLAGS)  -o $@ $(BOOT.o) $(OBJS) $(LIBS)
	@$(SIZE) $@

clean::; $(RM) $(BOOT.o) $(OBJS) $(LISTINGS)
clean::; $(RM) $(PGM) $(PGM).hex a.out

realclean:: clean
	clean
	$(RM) TAGS a.out core .make* .nse*
