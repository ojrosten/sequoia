#BUILD_ROOT   := ../build/native
#BUILD_ROOT   := ../build/gcc
BUILD_ROOT     := ../build/clang

TEST_BUILD_ROOT := $(BUILD_ROOT)/TestChamber
SEQ_BUILD_ROOT  := $(BUILD_ROOT)/SequoiaLib

TEST_BUILD_DIR  := $(BUILD_ROOT)/TestChamber/Tests
SEQ_BUILD_DIR   := $(BUILD_ROOT)/SequoiaLib/Source

TEST_DIR  := ../Tests
SEQ_DIR   := ../Source

TEST_SUB_DIRS := $(filter-out $(TEST_DIR), $(shell find $(TEST_DIR) -type d))
SEQ_SUB_DIRS  := $(filter-out $(SEQ_DIR),  $(shell find $(SEQ_DIR) -type d))

TEST_BUILD_SUB_DIRS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SUB_DIRS))
SEQ_BUILD_SUB_DIRS  := $(addprefix $(SEQ_BUILD_DIR)/, $(SEQ_SUB_DIRS))

.PHONY: clean subdirs

#CC := g++-9 -fmax-errors=1 -Wno-comment 
#CC := clang++ -ferror-limit=1 -Wno-missing-braces
CC := /usr/local/opt/llvm/bin/clang++ -ferror-limit=1

PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	-ftemplate-backtrace-limit=0	       \
	$(PREPROP)						       \
	$(patsubst %,-I%, $(TEST_SUB_DIRS))    \
	$(patsubst %,-I%, $(SEQ_SUB_DIRS))     \

CFLAGS += -I../TestCommon
CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/

EXE      := $(TEST_BUILD_ROOT)/TestChamber.exe
SEQ_LIB  := $(SEQ_BUILD_ROOT)/Sequoia.a

APP_SRC := TestChamberMain.cpp
APP_OBJ := $(TEST_BUILD_ROOT)/TestChamberMain.o
APP_DEP := $(TEST_BUILD_ROOT)/TestChamberMain.d

TEST_SRCS += $(shell find $(TEST_DIR)/TestFramework -name '*.cpp')
TEST_SRCS += $(shell find $(TEST_DIR)/Parsing -name '*.cpp')
TEST_OBJS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SRCS:%.cpp=%.o))
TEST_DEPS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SRCS:%.cpp=%.d))

SEQ_SRCS := $(shell find $(SEQ_DIR) -name '*.cpp')
SEQ_OBJS := $(addprefix $(SEQ_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.o))
SEQ_DEPS := $(addprefix $(SEQ_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.d))

LDFLAGS :=
LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

$(EXE) : $(APP_OBJ) $(TEST_OBJS) $(SEQ_LIB)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SEQ_LIB) : $(SEQ_OBJS)
	ar -cq $(SEQ_LIB) $^

-include $(APP_DEP)
-include $(TEST_DEPS)
-include $(SEQ_DEPS)

$(APP_OBJ) : $(APP_SRC)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(TEST_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(SEQ_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

subdirs :
	-mkdir $(BUILD_ROOT)
	-mkdir $(TEST_BUILD_ROOT)
	-mkdir $(SEQ_BUILD_ROOT)
	-mkdir $(TEST_BUILD_DIR)
	-mkdir $(SEQ_BUILD_DIR)
	-mkdir $(TEST_BUILD_SUB_DIRS)
	-mkdir $(SEQ_BUILD_SUB_DIRS)

clean :
	rm -r $(TEST_BUILD_ROOT)
	rm -r $(SEQ_BUILD_ROOT)
