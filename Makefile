ROOTDIR := .
TESTDIR := tests

SRCS    := $(shell find $(ROOTDIR) -name "*.cpp")
TESTS   := $(shell find $(TESTDIR) -name "*.sy" | sort)
TARGET  := ./sysyc

CXX      := g++
CXXFLAGS := -Wall -g
LD       := g++
LDFLFAGS :=

TESTARCH    := -march=rv32im -mabi=ilp32
TESTCC      := riscv64-elf-gcc
TESTCFLAGS  := -D__SYSY_TEST__ -Wall -O3 $(TESTARCH)
TESTLD      := riscv64-elf-gcc 
TESTLDFLAGS := $(TESTARCH)
TESTAS      := riscv64-elf-as
TESTASFLAGS := $(TESTARCH)
TESTEMU     := qemu-riscv32

MKDIR    := mkdir -p
ECHO     := echo -e
RM       := rm -r

DEPDIR := .dep
OBJDIR := .obj
BINDIR := .bin
TESTOBJDIR := $(OBJDIR)/$(TESTDIR)
TESTBINDIR := $(BINDIR)/$(TESTDIR)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
COMPILE  = $(CXX) $(DEPFLAGS) $(CXXFLAGS) -c
LINK     = $(LD) $(LDFLFAGS)

OBJFILES  := $(SRCS:%=$(OBJDIR)/%.o)
DEPFILES  := $(SRCS:%=$(DEPDIR)/%.d)
TESTSRCS  := $(TESTS:%.sy=%.c)
TESTBINS  := $(TESTS:%.sy=$(BINDIR)/%)
TESTOBJS  := $(TESTS:%=$(OBJDIR)/%.o) $(TESTSRCS:%=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: %
$(OBJDIR)/%.o: % $(DEPDIR)/%.d
	@$(MKDIR) $(dir $(OBJDIR)/$*)
	@$(MKDIR) $(dir $(DEPDIR)/$*)
	@$(ECHO) "\tCXX\t$<"
	@$(COMPILE) -o $@ $<

$(TESTOBJDIR)/%.c.o: $(TESTDIR)/%.c
	@$(MKDIR) $(dir $@)
	@$(ECHO) "\tCC  [T]\t$<"
	@$(TESTCC) $(TESTCFLAGS) -c -o $@ $<

$(TESTOBJDIR)/%.sy.o: $(TESTDIR)/%.sy $(TARGET)
	@$(MKDIR) $(dir $@)
	@$(ECHO) "\tSYC [T]\t$<"
	@$(TARGET) $< | $(TESTAS) $(TESTASFLAGS) -c -o $@

$(TESTBINDIR)/%: $(TESTOBJDIR)/%.c.o $(TESTOBJDIR)/%.sy.o
	@$(MKDIR) $(dir $@)
	@$(ECHO) "\tLD  [T]\t$(notdir $@)"
	@$(TESTLD) $(TESTLDFLAGS) -o $@ $^

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJFILES)
	@$(ECHO) "\tLD\t$@"
	@$(LINK) -o $@ $^

.PHONY: clean
clean:
	@$(RM) $(DEPDIR)
	@$(ECHO) "\tRM\t$(DEPDIR)"
	@$(RM) $(OBJDIR)
	@$(ECHO) "\tRM\t$(OBJDIR)"
	@$(RM) $(BINDIR)
	@$(ECHO) "\tRM\t$(BINDIR)"

.PHONY: tests
tests: $(TESTOBJS) $(TESTBINS)
	@$(ECHO) "Running tests..."
	@(for prog in $(TESTBINS); do $(TESTEMU) ./$$prog; done)

$(DEPFILES):
include $(wildcard $(DEPFILES))
