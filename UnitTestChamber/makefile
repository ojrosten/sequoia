BUILDROOT := ../build/native
BUILDDIR := $(BUILDROOT)/UnitTestChamber
LIBDIRS := ../Core ../Maths ../Experimental ../Algorithms ../Parsing ../UnitTestFramework
UTBUILDDIR := $(BUILDDIR)/UnitTests
UTDIR = ../UnitTests

UTSUBDIRS := $(filter-out ../UnitTests, $(shell find $(UTDIR) -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

CC := clang++
#CC := g++
# -D MINIMAL_GRAPH_TESTS
CFLAGS := -Wall -std=c++2a -O0 -ferror-limit=1 \
	  -Wno-missing-braces		       	   \
	  -ftemplate-backtrace-limit=0	       \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
      $(patsubst %,-I%, $(LIBSUBDIRS))
EXE := $(BUILDDIR)/UnitTestChamber.exe

UTSRCS := UnitTestChamberMain.cpp
UTSRCS += ../UnitTests/UnitTestFramework/UnitTestDiagnostics.cpp
UTSRCS += ../UnitTests/Parsing/CommandLineArgumentsTest.cpp
UTOBJS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.o))
UTDEPS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.d))

LIBSRCS := $(shell find $(LIBSUBDIRS) -name '*.cpp')
LIBOBJS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.o))
LIBDEPS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.d))

$(EXE) : $(UTOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(CHAMBERBUILDDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(UTBUILDDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(UTDEPS)
-include $(LIBDEPS)


.PHONY subdirs : $(BUILDSUBDIRS)
	-mkdir $(BUILDROOT)
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $(CHAMBERBUILDDIR)
	-mkdir $^

clean :
	rm -r $(BUILDDIR)
