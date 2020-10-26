#BUILD_ROOT   := ../build/native
#BUILD_ROOT   := ../build/gcc
BUILD_ROOT       := ../build/clang
BUILD_DIR        := $(BUILD_ROOT)/SequoiaLib
SOURCE_BUILD_DIR := $(BUILD_DIR)/Source
SOURCE_DIR       := .

SOURCE_SUB_DIRS := $(filter-out $(SOURCE_DIR), $(shell find $(SOURCE_DIR) -type d))

BUILD_SUB_DIRS += $(addprefix $(SOURCE_BUILD_DIR)/, $(SOURCE_SUB_DIRS))

.PHONY: all clean subdirs

#CC := g++-9 -fmax-errors=1 -Wno-comment 
#CC := clang++ -ferror-limit=1 -Wno-missing-braces
CC := /usr/local/opt/llvm/bin/clang++ -ferror-limit=1

CFLAGS := -Wall -Wextra -Wpedantic -std=c++2a -O0 \
	-ftemplate-backtrace-limit=0	       \
	$(PREPROP)						       \
	$(patsubst %,-I%, $(SOURCE_SUB_DIRS))  \

CFLAGS += -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/


SEQ_SRCS := $(shell find $(SOURCE_SUB_DIRS) -name '*.cpp')
SEQ_OBJS := $(addprefix $(SOURCE_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.o))
SEQ_DEPS := $(addprefix $(SOURCE_BUILD_DIR)/, $(SEQ_SRCS:%.cpp=%.d))
SEQ_LIB := $(BUILD_DIR)/Sequoia.a

LDFLAGS :=
LDFLAGS += -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib

$(SEQ_LIB) : $(SEQ_OBJS)
	ar -cq $(SEQ_LIB) $^

-include $(SEQ_DEPS)

$(SOURCE_BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -MMD -c $< -o $@

subdirs :
	-mkdir $(BUILD_ROOT)
	-mkdir $(BUILD_DIR)
	-mkdir $(SOURCE_BUILD_DIR)
	-mkdir $(BUILD_SUB_DIRS)

clean :
	rm -r $(BUILD_DIR)
