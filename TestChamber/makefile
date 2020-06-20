#BUILDROOT    := ../build/native
BUILDROOT    := ../build/gcc
BUILDDIR     := $(BUILDROOT)/TestChamber
LIBDIRS      := ../Core ../Maths ../Experimental ../Algorithms ../Parsing ../TestFramework ../TestCommon
UTBUILDDIR   := $(BUILDDIR)/Tests
UTDIR        := ../Tests
OUTPUTDIR    := ../output/TestSummaries/Tests

UTSUBDIRS  := $(filter-out ../Tests, $(shell find $(UTDIR) -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

OUTPUTSUBDIRS := $(addprefix $(OUTPUTDIR)/, $(UTSUBDIRS))

.PHONY: all clean subdirs

CC := g++-9 -fmax-errors=1 -Wno-comment 
#CC := clang++ -ferror-limit=1 -Wno-missing-braces

PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	  -ftemplate-backtrace-limit=0	       \
	  $(PREPROP)						   \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
      $(patsubst %,-I%, $(LIBSUBDIRS))

EXE    := $(BUILDDIR)/TestChamber.exe

UTSRCS := TestChamberMain.cpp
UTSRCS += $(shell find $(UTDIR)/TestFramework -name '*.cpp')
UTSRCS += $(shell find $(UTDIR)/Parsing -name '*.cpp')

UTOBJS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.o))
UTDEPS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.d))

LIBSRCS := $(shell find $(LIBSUBDIRS) -name '*.cpp')
LIBOBJS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.o))
LIBDEPS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.d))

$(EXE) : $(UTOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $^ -o $@

-include $(UTDEPS)
-include $(LIBDEPS)

$(UTBUILDDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@
#	echo "$<"

subdirs :
	-mkdir $(BUILDROOT)
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $(BUILDSUBDIRS)
	-mkdir $(OUTPUTSUBDIRS)

clean :
	rm -r $(BUILDDIR)
