#BUILD_ROOT   := ../build/native
#BUILD_ROOT   := ../build/gcc
BUILD_ROOT       := ../build/clang
BUILD_DIR        := $(BUILD_ROOT)/TestMinimal
SOURCE_BUILD_DIR := $(BUILD_DIR)/Source
UT_BUILD_DIR     := $(BUILD_DIR)/Tests
SOURCE_DIR       := ../Source
UT_DIR           := ../Tests

UT_SUB_DIRS     := $(filter-out ../Tests,  $(shell find $(UT_DIR) -type d))
SOURCE_SUB_DIRS := $(filter-out ../Source, $(shell find $(SOURCE_DIR) -type d))

BUILD_SUB_DIRS := $(addprefix $(UT_BUILD_DIR)/, $(UT_SUB_DIRS))
BUILD_SUB_DIRS += $(addprefix $(SOURCE_BUILD_DIR)/, $(SOURCE_SUB_DIRS))

.PHONY: all clean subdirs

#CC := g++-9 -fmax-errors=1 -Wno-comment 
#CC := clang++ -ferror-limit=1 -Wno-missing-braces
CC := /usr/local/opt/llvm/bin/clang++ -ferror-limit=1

PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	-ftemplate-backtrace-limit=0	       \
	$(PREPROP)						       \
	$(patsubst %,-I%, $(UT_SUB_DIRS))      \
	$(patsubst %,-I%, $(SOURCE_SUB_DIRS))  \

CFLAGS += -I../TestCommon
CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/


EXE    := $(BUILD_DIR)/TestMinimal.exe

UT_SRCS := TestMinimalMain.cpp
UT_SRCS += $(shell find $(UT_DIR)/Experimental -name '*.cpp')

UT_OBJS := $(addprefix $(UT_BUILD_DIR)/, $(UT_SRCS:%.cpp=%.o))
UT_DEPS := $(addprefix $(UT_BUILD_DIR)/, $(UT_SRCS:%.cpp=%.d))

SEQ_SRCS := $(shell find $(SOURCE_SUB_DIRS) -name '*.cpp')
SEQ_OBJS := $(addprefix $(UT_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.o))
SEQ_DEPS := $(addprefix $(UT_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.d))
SEQ_LIB := $(BUILD_DIR)/Sequoia.a

LDFLAGS :=
LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

$(EXE) : $(UT_OBJS) $(SEQ_LIB)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SEQ_LIB) : $(SEQ_OBJS)
	ar -cq $(SEQ_LIB) $^

-include $(UT_DEPS)
-include $(SEQ_DEPS)

$(UT_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

subdirs :
	-mkdir $(BUILD_ROOT)
	-mkdir $(BUILD_DIR)
	-mkdir $(SOURCE_BUILD_DIR)
	-mkdir $(UT_BUILD_DIR)
	-mkdir $(BUILD_SUB_DIRS)

clean :
	rm -r $(BUILD_DIR)
