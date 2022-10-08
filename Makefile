NAME			= klalloc
EXT				= so
CC				= clang
RM				= rm -f
MKDIR			= mkdir -p
SYMLINK			= ln -fs
DEBUG			= 1

ifeq ($(DEBUG), 1)
	DEBUG_FLAGS	= -fsanitize=address -g
else
	MAKEFLAGS	= -j --output-sync=recurse --no-print-directory
	DEBUG_FLAGS	= -O2 -flto -fPIE
endif

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

WARNING_FLAGS   = -Wall -Wextra -Werror -Wpointer-arith -Wfloat-equal -fpermissive
PROTECT_FLAGS	= -fno-exceptions -fstack-protector-all
COMMON_FLAGS	= -std=c11 -MMD -pipe
CFLAGS			= $(DEBUG_FLAGS) $(COMMON_FLAGS) $(PROTECT_FLAGS) $(WARNING_FLAGS)

BIN_DIR			= bin
SRC_DIR			= src
TESTS_DIR		= tests
BUILD_DIR		= build
INC_DIR			= include

SRCS			= src/klalloc.c\
					src/slab_alloc.c\
					src/utils.c\

SYMLINK_NAME	= lib$(NAME).$(EXT)
SYMLINK_PATH	= $(BIN_DIR)/lib$(NAME).$(EXT)

BIN_PATH		= $(BIN_DIR)/lib$(NAME)_$(HOSTTYPE).$(EXT)
OBJS			= $(subst $(SRC_DIR), $(BUILD_DIR), $(SRCS:%.c=%.o))
DEPS			= $(OBJS:.o=.d)

VPATH			= $(SRC_DIR):$(INC_DIR):$(BUILD_DIR)

TESTS_SRC_DIR	= $(TESTS_DIR)/$(SRC_DIR)
TESTS_INC_DIR	= $(TESTS_DIR)/$(INC_DIR)
TESTS_BUILD_DIR	= $(TESTS_DIR)/$(BUILD_DIR)

TESTS_BIN_DIR	= $(TESTS_DIR)/bin
TESTS_BIN		= $(TESTS_BIN_DIR)/exec_tests

TESTS_SRCS		= $(shell find $(TESTS_SRC_DIR) -name '*.c')
TESTS_OBJS		= $(subst $(TESTS_SRC_DIR), $(TESTS_BUILD_DIR), $(TESTS_SRCS:%.c=%.o))
TEST_DEPS		= $(TESTS_OBJS:.o=.d)

CURRENT_DIR		= $(shell pwd)

all:				$(BIN_PATH)

$(BIN_PATH):		$(OBJS)
					@if [ ! -d $(dir $@) ] ; then $(MKDIR) $(dir $@); fi
					$(CC) -shared $(OBJS) -o $(BIN_PATH)
					$(SYMLINK) $(BIN_PATH) $(SYMLINK_PATH)

$(BUILD_DIR)/%.o:  $(SRC_DIR)/%.c
					@if [ ! -d $(dir $@) ] ; then $(MKDIR) $(dir $@); fi
					$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@

run_tests:			$(TESTS_BIN)
					./$<

run_tests_leaks:	$(TESTS_BIN)
					leaks --atExit -- ./$<

build_tests:		$(TESTS_BIN)

$(TESTS_BUILD_DIR)/%.o: $(TESTS_SRC_DIR)/%.c
					@if [ ! -d $(dir $@) ] ; then $(MKDIR) $(dir $@); fi
					$(CC) $(CFLAGS) -I $(INC_DIR) -I $(TESTS_INC_DIR) -c $< -o $@

$(TESTS_BIN):   	$(TESTS_OBJS) $(OBJS)
					@if [ ! -d $(dir $@) ] ; then $(MKDIR) $(dir $@); fi
					$(CC) $(CFLAGS) -I $(INC_DIR) -I $(TESTS_INC_DIR) $< $(OBJS) -o $@

tests_clean:
					$(RM) $(TESTS_OBJS)
					$(RM) -rd $(TESTS_BUILD_DIR)

tests_fclean:		tests_clean
					$(RM) $(TESTS_BIN)
					$(RM) -rd $(TESTS_BIN_DIR)

clean:				tests_clean
					$(RM) $(OBJS)
					$(RM) $(DEPS)
					$(RM) -rd $(BUILD_DIR)

fclean:				clean tests_fclean
					$(RM) $(BIN_PATH)
					$(RM) -rd $(BIN_DIR)

re:
					$(MAKE) fclean
					$(MAKE) all

-include $(DEPS)
-include $(TEST_DEPS)
.PHONY:				all, clean, fclean, re