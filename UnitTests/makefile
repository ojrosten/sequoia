BUILDDIR := ../build
LIBDIRS := ../Core ../Maths ../Experimental ../Algorithms
UTBUILDDIR := $(BUILDDIR)/UnitTests

UTSUBDIRS := $(filter-out ., $(shell find . -type d))
LIBSUBDIRS := $(shell find $(LIBDIRS) -type d)

BUILDSUBDIRS := $(addprefix $(UTBUILDDIR)/, $(UTSUBDIRS))
BUILDSUBDIRS += $(addprefix $(UTBUILDDIR)/, $(LIBSUBDIRS))

CC := clang++
#CC := g++
CFLAGS := -Wall -std=c++1z -O0 -ferror-limit=1 \
	  -Wno-missing-braces		       \
	  $(patsubst %,-I%, $(UTSUBDIRS))      \
          $(patsubst %,-I%, $(LIBSUBDIRS))
EXE := $(UTBUILDDIR)/UnitTest.exe

UTSRCS := $(shell find . -name '*.cpp' ! -name TestUnitTesterMain.cpp)
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
	-mkdir $(BUILDDIR)
	-mkdir $(UTBUILDDIR)
	-mkdir $^

clean :
	rm -r $(BUILDDIR)
