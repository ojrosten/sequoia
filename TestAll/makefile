#BUILDROOT    := ../build/native
#BUILDROOT    := ../build/gcc
BUILDROOT    := ../build/clang
BUILDDIR   := $(BUILDROOT)/TestAll
LIBDIRS    := ../Core ../Maths ../Experimental ../Algorithms ../Parsing ../TestFramework ../TestCommon
UTBUILDDIR := $(BUILDDIR)/Tests
UTDIR      := ../Tests
OUTPUTDIR    := ../output/TestSummaries/Tests

UTSUBDIRS  := $(filter-out ../Tests, $(shell find $(UTDIR) -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

OUTPUTSUBDIRS := $(addprefix $(OUTPUTDIR)/, $(UTSUBDIRS))

.PHONY: all clean subdirs

#CC := g++-9 -fmax-errors=1 -Wno-comment 
#CC := clang++ -ferror-limit=1 -Wno-missing-braces
CC := /usr/local/opt/llvm/bin/clang++ -ferror-limit=1 -Wno-missing-braces

PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	  -ftemplate-backtrace-limit=0	       \
	  $(PREPROP)						   \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
      $(patsubst %,-I%, $(LIBSUBDIRS))

CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/

EXE := $(BUILDDIR)/TestAll.exe

UTSRCS := TestMain.cpp
UTSRCS += $(shell find $(UTDIR) -name '*.cpp')
UTOBJS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.o))
UTDEPS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.d))

LIBSRCS := $(shell find $(LIBSUBDIRS) -name '*.cpp')
LIBOBJS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.o))
LIBDEPS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.d))

LDFLAGS :=
LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

$(EXE) : $(UTOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(UTBUILDDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(UTDEPS)
-include $(LIBDEPS)

subdirs :
	-mkdir $(BUILDROOT)
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $(BUILDSUBDIRS)
	-mkdir $(OUTPUTSUBDIRS)

clean :
	rm -r $(BUILDDIR)
