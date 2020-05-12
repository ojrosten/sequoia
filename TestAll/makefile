BUILDROOT  := ../build/native
BUILDDIR   := $(BUILDROOT)/TestAll
LIBDIRS    := ../Core ../Maths ../Experimental ../Algorithms ../Parsing ../TestFramework ../TestCommon
UTBUILDDIR := $(BUILDDIR)/Tests
UTDIR      := ../Tests

UTSUBDIRS  := $(filter-out ../Tests, $(shell find $(UTDIR) -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

.PHONY: all clean subdirs

CC := clang++
PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS
#CC := g++
CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 -ferror-limit=1 \
	  -Wno-missing-braces		       	   \
	  -ftemplate-backtrace-limit=0	       \
	  $(PREPROP)						   \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
      $(patsubst %,-I%, $(LIBSUBDIRS))
EXE := $(BUILDDIR)/TestAll.exe

UTSRCS := TestMain.cpp
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

subdirs : 
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $(BUILDSUBDIRS)

clean :
	rm -r $(BUILDDIR)
