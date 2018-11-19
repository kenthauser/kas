CXXFLAGS += -std=c++17 -g -pedantic -I.
CXXFLAGS +=  -isystem meta/include -isystem /usr/local/include
CXXFLAGS += -O2
##CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
# CXXFLAGS += -O2 -isystem ../boost_1_58_0
# CXX = g++-7

# CXXFLAGS += -DPRINT_EXPR_INFO

# clang options to create .deps file & .phony target
OUTPUT_OPTION = -MMD -MP
# OUTPUT_OPTION = -MD -MP
# CXXFLAGS += -DBOOST_SPIRIT_X3_DEBUG
LINK.o = $(LINK.cc)
# LINK.o += -Xlinker -v
CXXFLAGS += -ftemplate-backtrace-limit=0

.PHONY: all clean tests $(TESTS)

all: po_test

po_test: po_test.o
	$(LINK.cc) -o $@ $?
	./$@


# include .deps files
-include $(wildcard *.d)

