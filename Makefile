CXXFLAGS += -std=c++17 -I. -g
CXXFLAGS += -pedantic
CXXFLAGS += -isystem spirit/include -isystem meta/include -isystem boost \
                                    -isystem /opt/homebrew/include       \
                                    -isystem /usr/local/include          \
                                    
CXXFLAGS += -ftemplate-depth=4096
CXXFLAGS += -Wno-deprecated-declarations

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

.PHONY: all clean distclean tests clone-boost $(TESTS)

all: config.status $(TESTS)

VPATH = parser:expr:kas_core:bsd:kbfd:test:kas_exec

OBJS =  kas_core.o expr.o parser.o kbfd.o
OBJS += bsd.o       # assume BSD pseudos

TEST_EXPR_ARGS  = test_files/expr_tests
TEST_PARSE_ARGS = test_files/parse_tests
TEST_EMIT_ARGS  = test_files/emit_tests

#CXXFLAGS += -DTRACE_DO_FRAG=3
#expr.o : CXXFLAGS += -DPRINT_EXPR_INFO
#expr.o : CXXFLAGS += -DEXPR_TRACE_EVAL
#CXXFLAGS += -DTOKEN_TRACE
#CXXFLAGS += -DTRACE_ARG_SERIALIZE
#CXXFLAGS += -DTRACE_CORE_RELOC
#m68k.o : CXXFLAGS += -DTRACE_M68K_PARSE
#CXXFLAGS += -DTRACE_ERROR_HANDLER

# make executable from python script
%: %.py
	cp $^ $@; chmod +x $@

# make all .o files depend on config.status
%.o: %.cc config.status
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

-include machine_makefile.inc

kas_expr_test: kas_expr_test.o $(OBJS)
	$(LINK.o) -o $@ $^  $(LIBS)
kas_parse_test: kas_parse_test.o $(OBJS)
	$(LINK.o) -o $@ $^  $(LIBS)
kas_emit_test: kas_emit_test.o $(OBJS)
	$(LINK.o) -o $@ $^  $(LIBS)


test_expr: kas_expr_test
	./$< $(TEST_EXPR_ARGS)

test_parse: kas_parse_test
	./$< $(TEST_PARSE_ARGS)

test_emit: kas_emit_test
	./$< $(TEST_EMIT_ARGS)


as: kas_main.o $(OBJS); $(LINK.o) -o $@ $^

test_kas: as; ./$< $(TEST_KAS_ARGS)

overwrite: $(ALL_TESTS)
	-./kas_expr_test  --overwrite $(TEST_EXPR_ARGS)
	-./kas_parse_test --overwrite $(TEST_PARSE_ARGS)
	-./kas_emit_test  --overwrite $(TEST_EMIT_ARGS)

config.status: configure
	@$(MAKE) boost
	git submodule update --init
	./configure -h
	./configure

boost:
	@echo "*** Boost required for compilation"
	@echo "*** Use package manager to install boost"
	@echo "*** Alternately use target 'clone-boost' to download from github.com"
	@echo "*** NB: download uses 2GB of disk"

distclean: clean
	git submodule deinit --all
	@if [ -f ./configure ]; then ./configure -d;fi
	$(RM) configure


# download boost libraries referenced (directly or indirectly) by spirit
BOOST_LIBS = libs/mpl libs/type_index libs/container_hash libs/static_assert \
             libs/throw_exception libs/assert libs/core libs/type_traits \
             libs/integer libs/detail libs/preprocessor libs/fusion \
             libs/utility libs/variant libs/move libs/range libs/optional \
             libs/regex libs/predef libs/iterator libs/concept_check \
             libs/math libs/typeof \
             libs/spirit

XTRA_LIBS  = libs/filesystem libs/locale libs/system libs/io libs/smart_ptr
 

clone-boost:
	-git clone http://github.org/boostorg/boost.git
	cd boost; git submodule update --init tools libs/config
	cd boost; git submodule update --init $(BOOST_LIBS) $(XTRA_LIBS)
	cd boost; ./bootstrap.sh
	cd boost; ./b2 headers

clean:
	$(RM) $(TARGET) kas_expr_test kas_parse_test kas_emit_test *.o *.d as

# include .deps files
-include $(wildcard *.d)
