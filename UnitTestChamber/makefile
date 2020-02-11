BUILDROOT    := ../build/native
BUILDDIR     := $(BUILDROOT)/UnitTestChamber
LIBDIRS      := ../Core ../Maths ../Experimental ../Algorithms ../Parsing ../UnitTestFramework
UTBUILDDIR   := $(BUILDDIR)/UnitTests
UTDIR        := ../UnitTests

UTSUBDIRS  := $(filter-out ../UnitTests, $(shell find $(UTDIR) -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

.PHONY: all clean subdirs

CC := clang++
#CC := g++
# -D MINIMAL_GRAPH_TESTS
CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 -ferror-limit=1 \
	  -Wno-missing-braces		       	   \
	  -ftemplate-backtrace-limit=0	       \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
      $(patsubst %,-I%, $(LIBSUBDIRS))

OUTPUT := $(BUILDDIR)/UnitTestChamberOutput.txt
EXE    := $(BUILDDIR)/UnitTestChamber.exe

UTSRCS := UnitTestChamberMain.cpp
UTSRCS += $(shell find $(UTDIR)/UnitTestFramework -name '*.cpp')
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
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $(BUILDSUBDIRS)

clean :
	rm -r $(BUILDDIR)
