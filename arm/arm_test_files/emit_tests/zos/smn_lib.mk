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
# $Id: smn_lib.mk,v 1.1.1.1 2000/10/18 04:29:56 kent Exp $
#
#**********************************************************
#
# Recursive Makefile instructions for Z80 sources
#
#**********************************************************

SUFFIXES = .o .c .s .S .ln .l  .y .h .sh .z80 .lst .hex .a
.SUFFIXES: $(SUFFIXES)
.KEEP_STATE:		# permanently enable feature

#
# declare "standard" targets
#

# create default target (all)

SUBDIRS =
#DIR_DOWN = "ZROOT=../${ZROOT} DIR=../${DIR}"

all install clean realclean::
	@set X ${SUBDIRS}; shift; 				\
	 for i do \
		echo making $@ in $$i;				\
		(cd $$i; ${MAKE} ${MFLAGS} ${DIR_DOWN} $@);	\
	done

clean::;  $(RM) a.out core
realclean::
	$(RM) TAGS a.out core .make* .nse*
	clean
