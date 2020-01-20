BUILDROOT := ../build/native
BUILDDIR := $(BUILDROOT)/UnitTestAll
LIBDIRS := ../Core ../Maths ../Experimental ../Algorithms ../Parsing ../UnitTestFramework
UTBUILDDIR := $(BUILDDIR)/UnitTests
UTDIR = ../UnitTests

UTSUBDIRS := $(filter-out ../UnitTests, $(shell find $(UTDIR) -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

CC := clang++
PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS
#CC := g++
CFLAGS := -Wall -std=c++2a -O0 -ferror-limit=1 \
	  -Wno-missing-braces		       	   \
	  -ftemplate-backtrace-limit=0	       \
	  $(PREPROP)						   \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
      $(patsubst %,-I%, $(LIBSUBDIRS))
EXE := $(BUILDDIR)/UnitTest.exe

UTSRCS := UnitTestMain.cpp
UTSRCS += $(shell find $(UTDIR) -name '*.cpp')
UTOBJS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.o))
UTDEPS := $(addprefix $(UTBUILDDIR)/, $(UTSRCS:%.cpp=%.d))

LIBSRCS := $(shell find $(LIBSUBDIRS) -name '*.cpp')
LIBOBJS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.o))
LIBDEPS := $(addprefix $(UTBUILDDIR)/, $(LIBSRCS:%.cpp=%.d))

$(EXE) : $(UTOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(UTBUILDDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@


-include $(UTDEPS)
-include $(LIBDEPS)

.PHONY subdirs : $(BUILDSUBDIRS)
	-mkdir $(BUILDROOT)
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $^

clean :
	rm -r $(BUILDDIR)
