#BUILD_ROOT   := ../build/native
#BUILD_ROOT   := ../build/gcc
SEQUOIA_ROOT := /Users/Claire/Dropbox/Sequoia
BUILD_ROOT   := ../build/clang

TEST_BUILD_ROOT := $(BUILD_ROOT)/TestAll
PRJ_BUILD_ROOT  := $(BUILD_ROOT)/ProjectLib

TEST_BUILD_DIR  := $(BUILD_ROOT)/TestAll/Tests
PRJ_BUILD_DIR   := $(BUILD_ROOT)/ProjectLib/Source

TEST_DIR  := ../Tests
PRJ_DIR   := ../Source
SEQ_DIR   := $(SEQUOIA_ROOT)/Source

SEQ_LIB := $(SEQUOIA_ROOT)/build/$(BUILD_ROOT)/SequoiaLib/Sequoia.a

TEST_SUB_DIRS := $(filter-out $(TEST_DIR), $(shell find $(TEST_DIR) -type d))
PRJ_SUB_DIRS  := $(filter-out $(PRJ_DIR),  $(shell find $(PRJ_DIR) -type d))
SEQ_SUB_DIRS  := $(filter-out $(SEQ_DIR),  $(shell find $(SEQ_DIR) -type d))

TEST_BUILD_SUB_DIRS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SUB_DIRS))
PRJ_BUILD_SUB_DIRS  := $(addprefix $(PRJ_BUILD_DIR)/, $(PRJ_SUB_DIRS))

.PHONY: clean subdirs

#CC := g++-9
#CC := clang++
CC := /usr/local/opt/llvm/bin/clang++

PREPROP :=

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	-ftemplate-backtrace-limit=0	       \
	$(PREPROP)						       \
	$(patsubst %,-I%, $(TEST_SUB_DIRS))    \
	$(patsubst %,-I%, $(PRJ_SUB_DIRS))     \
	$(patsubst %,-I%, $(SEQ_SUB_DIRS))     \

CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/

CFLAGS +=  -ferror-limit=1 -Wno-missing-braces
#CFLAGS += -fmax-errors=1 -Wno-comment

EXE      := $(TEST_BUILD_ROOT)/TestAll.exe

APP_SRC := TestMain.cpp
APP_OBJ := $(TEST_BUILD_ROOT)/TestMain.o
APP_DEP := $(TEST_BUILD_ROOT)/TestMain.d

TEST_SRCS += $(shell find $(TEST_DIR) -name '*.cpp')
TEST_OBJS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SRCS:%.cpp=%.o))
TEST_DEPS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SRCS:%.cpp=%.d))

PRJ_SRCS := $(shell find $(PRJ_DIR) -name '*.cpp')
PRJ_OBJS := $(addprefix $(PRJ_BUILD_DIR)/, $(PRJ_SRCS:%.cpp=%.o))
PRJ_DEPS := $(addprefix $(PRJ_BUILD_DIR)/, $(PRJ_SRCS:%.cpp=%.d))

LDFLAGS :=
LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

$(EXE) : $(APP_OBJ) $(TEST_OBJS) $(PRJ_OBJS)
	$(CC) $(CFLAGS) $^ $(SEQ_LIB) -o $@ $(LDFLAGS)

-include $(APP_DEP)
-include $(TEST_DEPS)
-include $(PRJ_DEPS)

$(APP_OBJ) : $(APP_SRC)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(TEST_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(PRJ_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

subdirs :
	-mkdir $(BUILD_ROOT)
	-mkdir $(TEST_BUILD_ROOT)
	-mkdir $(PRJ_BUILD_ROOT)
	-mkdir $(TEST_BUILD_DIR)
	-mkdir $(PRJ_BUILD_DIR)
	-mkdir $(TEST_BUILD_SUB_DIRS)
	-mkdir $(PRJ_BUILD_SUB_DIRS)

clean :
	rm -r $(TEST_BUILD_ROOT)
	rm -r $(PRJ_BUILD_ROOT)

