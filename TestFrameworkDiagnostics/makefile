#BUILD_ROOT   := ../build/native
#BUILD_ROOT   := ../build/gcc
BUILD_ROOT     := ../build/clang

TEST_BUILD_ROOT := $(BUILD_ROOT)/TestFrameworkDiagnostics
SEQ_BUILD_ROOT  := $(BUILD_ROOT)/SequoiaLib

TEST_BUILD_DIR  := $(BUILD_ROOT)/TestFrameworkDiagnostics/Tests
SEQ_BUILD_DIR   := $(BUILD_ROOT)/SequoiaLib/Source

TEST_DIR  := ../Tests
SEQ_DIR   := ../Source/sequoia

TEST_SUB_DIRS := $(filter-out $(TEST_DIR), $(shell find $(TEST_DIR) -type d))
SEQ_SUB_DIRS  := $(filter-out $(SEQ_DIR),  $(shell find $(SEQ_DIR) -type d))

TEST_BUILD_SUB_DIRS := $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SUB_DIRS))
SEQ_BUILD_SUB_DIRS  := $(addprefix $(SEQ_BUILD_DIR)/, $(SEQ_SUB_DIRS))

.PHONY: clean subdirs

CC := /usr/local/Cellar/gcc/11.2.0_3/bin/g++-11
#CC := clang++ 
#CC := /usr/local/opt/llvm/bin/clang++

PREPROP :=
#PREPROP += -D MINIMAL_GRAPH_TESTS

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	-ftemplate-backtrace-limit=0	       \
	$(PREPROP)						       \

CFLAGS += -I../TestCommon -I../Source -I../Tests
#CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/
CFLAGS += -I/usr/local/Cellar/gcc/11.2.0_3/include -I/usr/local/Cellar/gcc/11.2.0_3/include/c++/11/


#CFLAGS +=  -ferror-limit=1 -Wno-missing-braces -fsanitize=address
CFLAGS += -v -fmax-errors=1 -Wno-comment

EXE      := $(TEST_BUILD_ROOT)/TestFrameworkDiagnostics
SEQ_LIB  := $(SEQ_BUILD_ROOT)/Sequoia.a

APP_SRC := TestFrameworkDiagnosticsMain.cpp
APP_OBJ := $(TEST_BUILD_ROOT)/TestFrameworkDiagnosticsMain.o
APP_DEP := $(TEST_BUILD_ROOT)/TestFrameworkDiagnosticsMain.d

TEST_SRCS += $(shell find $(TEST_DIR)/TestFramework -name '*.cpp')
TEST_SRCS += $(shell find $(TEST_DIR)/Parsing -name '*.cpp')
TEST_SRCS += $(shell find $(TEST_DIR)/Runtime -name '*.cpp')
TEST_SRCS += $(shell find $(TEST_DIR)/TextProcessing -name '*.cpp')

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
	ar -cr $(SEQ_LIB) $^

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
	mkdir -p $(BUILD_ROOT)
	mkdir -p $(TEST_BUILD_ROOT)
	mkdir -p $(SEQ_BUILD_ROOT)
	mkdir -p $(TEST_BUILD_DIR)
	mkdir -p $(SEQ_BUILD_DIR)
	mkdir -p $(TEST_BUILD_SUB_DIRS)
	mkdir -p $(SEQ_BUILD_SUB_DIRS)

clean :
	rm -r $(TEST_BUILD_ROOT)
	rm -r $(SEQ_BUILD_ROOT)
