AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc$(CC_VERSION)

OBJ_DIR = obj
SRC_DIR = src
BIN_DIR = bin
LIB_DIR = lib
TEST_DIR = test/src
BRADTEST_DIR = bradtest

LDFLAGS = -ggdb
ARFLAGS = -r
CCFLAGS = -ggdb -Wall -Wextra -Werror -Wswitch-default -Wwrite-strings \
	-O3 -Iinclude -Itest/include -std=gnu99 $(CFLAGS)

FLORAL_C_SRCS = main.c thread.c
FLORAL_S_SRCS = context_switch.S
FLORAL_OBJS = $(FLORAL_C_SRCS:%.c=$(OBJ_DIR)/%.o) $(FLORAL_S_SRCS:%.S=$(OBJ_DIR)/%.o)

TEST_SRCS = test.c test_utils.c \
	phase1_tests.c phase2_tests.c phase3_tests.c phase4_tests.c phase5_tests.c

BRADTEST_SRCS = bradtest1.c

TEST_OBJS = $(TEST_SRCS:%.c=$(OBJ_DIR)/%.o)
BRADTEST_OBJS = $(BRADTEST_SRCS:%.c=$(OBJ_DIR)/%.o)

LIB_NAME = floral
LIB = $(LIB_DIR)/lib$(LIB_NAME).a
TEST_BIN = $(BIN_DIR)/test

BRADTEST_BIN = $(BIN_DIR)/bradtest

SUBMIT_TAR = hw2.tar.gz

.PHONY: all clean test submission

vpath % $(SRC_DIR) $(TEST_DIR) $(BRADTEST_DIR)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -c $< -o $@

$(LIB): $(FLORAL_OBJS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(TEST_BIN): $(TEST_OBJS) $(LIB)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -L$(LIB_DIR) -lfloral -o $@ $^

$(BRADTEST_BIN): $(BRADTEST_OBJS) $(LIB)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -L$(LIB_DIR) -lfloral -o $@ $^

$(SUBMIT_TAR):
	tar -zcf $@ $(SRC_DIR)

all: $(LIB)

test: $(TEST_BIN)
	@$(TEST_BIN)

bradtest: $(BRADTEST_BIN)
	@$(BRADTEST_BIN)

submission: $(SUBMIT_TAR)
	@echo "Your submission file "$^" was successfully created."
	@echo "Submit it on collab."

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) $(SUBMIT_TAR)
