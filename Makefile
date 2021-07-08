CXXFLAGS += -std=c++17 -I. -g
//CXXFLAGS += -std=c++2a -I. -g
CXXFLAGS += -pedantic
CXXFLAGS += -isystem spirit/include -isystem meta/include   \
                                    -isystem /opt/homebrew/include \
                                    -isystem /usr/local/include
                                    
CXXFLAGS += -ftemplate-depth=4096
#CXXFLAGS += -O2 -fomit-frame-pointer
#CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
# CXXFLAGS += -O2 -isystem ../boost_1_58_0
#CXX = g++-8
# CXXFLAGS += -DPRINT_EXPR_INFO

# clang options to create .deps file & .phony target
OUTPUT_OPTION = -MMD -MP
# OUTPUT_OPTION = -MD -MP
# CXXFLAGS += -DBOOST_SPIRIT_X3_DEBUG
LINK.o = $(LINK.cc) -L /opt/homebrew/lib
# LINK.o += -Xlinker -v
CXXFLAGS += -ftemplate-backtrace-limit=0

ALL_TESTS = test_expr test_parse test_emit
TESTS = $(ALL_TESTS)
#TESTS = test_expr
#TESTS = test_parse
TESTS = test_emit
# TESTS = vtable-test
# TESTS = str
#TESTS = as
#TESTS = tagging_demo

.PHONY: all clean tests $(TESTS) test_kas

all: $(TESTS)

vtable-test: vtable-test.cc
	$(LINK.cc) -o $@ $?
	./$@

str: str.cc
	$(LINK.cc) -o $@ $?
	./$@

VPATH = parser:expr:kas_core:bsd:kbfd:test:kas_exec

OBJS =  kas_core.o expr.o parser.o kbfd.o
OBJS += bsd.o
#OBJS += m68k.o
#OBJS += z80.o
#OBJS += arm.o

LIBS = -lboost_regex -lboost_filesystem -lboost_system

TEST_EXPR_ARGS  = test/expr_tests
TEST_PARSE_ARGS = test/parse_tests
TEST_EMIT_ARGS  = test/emit_tests

#CXXFLAGS += -DTRACE_DO_FRAG=3
#expr.o : CXXFLAGS += -DPRINT_EXPR_INFO
#expr.o : CXXFLAGS += -DEXPR_TRACE_EVAL
#CXXFLAGS += -DTOKEN_TRACE
#CXXFLAGS += -DTRACE_ARG_SERIALIZE
#m68k.o : CXXFLAGS += -DTRACE_M68K_PARSE
#CXXFLAGS += -DTRACE_ERROR_HANDLER


include machine_makefile.inc

#kas_expr_test: kas_expr_test.o expr.o kas_core.o bsd.o m68k.o m68k_defns.o
kas_expr_test: kas_expr_test.o $(OBJS)
	$(LINK.o) -o $@ $^  -lboost_regex $(LIBS)
kas_parse_test: kas_parse_test.o $(OBJS)
	$(LINK.o) -o $@ $^  -lboost_regex $(LIBS)
kas_emit_test: kas_emit_test.o $(OBJS)
	$(LINK.o) -o $@ $^  -lboost_regex $(LIBS)


test_expr: kas_expr_test
	./$< $(TEST_EXPR_ARGS)

test_parse: kas_parse_test
	./$< $(TEST_PARSE_ARGS)

test_emit: kas_emit_test
	./$< $(TEST_EMIT_ARGS)


as: kas_main.o $(OBJS); $(LINK.o) -o $@ $^ -lboost_system -lboost_filesystem
tagging_demo : tagging_demo.o; $(LINK.o) -o $@ $^;./tagging_demo

test_kas: as; ./$< $(TEST_KAS_ARGS)

overwrite: $(ALL_TESTS)
	-./kas_expr_test  --overwrite $(TEST_EXPR_ARGS)
	-./kas_parse_test --overwrite $(TEST_PARSE_ARGS)
	-./kas_emit_test  --overwrite $(TEST_EMIT_ARGS)

clean:
	$(RM) $(TARGET) kas_expr_test kas_parse_test kas_emit_test *.o *.d as

# include .deps files
-include $(wildcard *.d)
