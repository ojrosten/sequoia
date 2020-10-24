#BUILD_ROOT := ../build/native
#BUILD_ROOT := ../build/gcc
SEQUOIA_ROOT     := /Users/Claire/Dropbox/Sequoia
BUILD_ROOT       := ../build/clang
BUILD_DIR        := $(BUILD_ROOT)/TestMain
SOURCE_BUILD_DIR := $(BUILD_DIR)/Source
UT_BUILD_DIR     := $(BUILD_DIR)/Tests
SEQ_BUILD_DIR    := $(BUILD_DIR)/Sequoia
SOURCE_DIR       := ../Source
UT_DIR           := ../Tests
SEQ_SRC_DIR      := $(SEQUOIA_ROOT)/Source

UT_SUB_DIRS      := $(filter-out $(UT_DIR), $(shell find $(UT_DIR) -type d))
SOURCE_SUB_DIRS  := $(filter-out $(SOURCE_DIR), $(shell find $(SOURCE_DIR) -type d))
SEQ_SRC_SUB_DIRS := $(filter-out $(SEQ_SRC_DIR)$, $(shell find $(SEQ_SRC_DIR) -type d))

BUILD_SUB_DIRS := $(addprefix $(UT_BUILD_DIR)/, $(UT_SUB_DIRS))
BUILD_SUB_DIRS += $(addprefix $(SOURCE_BUILD_DIR)/, $(SOURCE_SUB_DIRS))
BUILD_SUB_DIRS += $(addprefix $(SEQ_BUILD_DIR)/, $(SEQ_SRC_SUB_DIRS))

.PHONY: all clean subdirs

#CC := g++-9 -fmax-errors=1 -Wno-comment 
#CC := clang++ -ferror-limit=1 -Wno-missing-braces
CC := /usr/local/opt/llvm/bin/clang++ -ferror-limit=1

PREPROP :=

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	  -ftemplate-backtrace-limit=0	       \
	  $(PREPROP)						   \
	  $(patsubst %,-I%, $(UT_SUB_DIRS))      \
      $(patsubst %,-I%, $(SOURCE_SUB_DIRS)) \
	  $(patsubst %,-I%, $(SEQ_SRC_SUB_DIRS))

CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/

EXE := $(BUILD_DIR)/TestMain.exe

UT_SRCS := TestMain.cpp
UT_SRCS += $(shell find $(UT_DIR) -name '*.cpp')

UT_OBJS := $(addprefix $(UT_BUILD_DIR)/, $(UT_SRCS:%.cpp=%.o))
UT_DEPS := $(addprefix $(UT_BUILD_DIR)/, $(UT_SRCS:%.cpp=%.d))

SRCS := $(shell find $(SOURCE_SUB_DIRS) -name '*.cpp')
OBJS := $(addprefix $(UT_BUILD_DIR)/, $(LIB_SRCS:%.cpp=%.o))
DEPS := $(addprefix $(UT_BUILD_DIR)/, $(LIB_SRCS:%.cpp=%.d))

SEQ_SRCS := $(shell find $(SEQ_SRC_SUB_DIRS) -name '*.cpp')
SEQ_OBJS := $(addprefix $(UT_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.o))
SEQ_DEPS := $(addprefix $(UT_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.d))

LDFLAGS :=
LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

$(EXE) : $(UT_OBJS) $(OBJS) $(SEQ_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(UT_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(UT_DEPS)
-include $(DEPS)
-include $(SEQ_DEPS)

subdirs :
	-mkdir $(BUILD_ROOT)
	-mkdir $(BUILD_DIR)
	-mkdir $(SOURCE_BUILD_DIR)
	-mkdir $(UT_BUILD_DIR)
	-mkdir $(SEQ_BUILD_DIR)
#	-mkdir $(BUILD_SUB_DIRS)

clean :
	rm -r $(BUILD_DIR)
