OPTIMIZER := libLoopFusion.so
ARRAY_SIZE := 1000

BIN_DIR := bin
LIB_DIR := lib
OBJ_DIR := obj
TEST_DIR := test

TESTFILE_SRC := $(TEST_DIR)/Loop.c
TESTFILE_LL := $(TESTFILE_SRC:%.c=%.ll)
TESTFILE_OPT_LL := $(TESTFILE_SRC:%.c=%.opt.ll)
TESTFILE_OBJ := $(TESTFILE_SRC:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)
TESTFILE_OPT_OBJ := $(TESTFILE_OBJ:%.o=%.opt.o)

MAIN_SRC := $(TEST_DIR)/main.c
MAIN_OBJ := $(MAIN_SRC:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)

LIB_SRC := $(LIB_DIR)/LoopFusion.cpp
LIB_INCLUDE := $(LIB_SRC:%.cpp=%.hpp)
LIB_OBJ := $(LIB_SRC:$(LIB_DIR)/%.cpp=$(OBJ_DIR)/%.o)

BIN := $(BIN_DIR)/main
BIN_OPT := $(BIN)-opt

CC = /usr/bin/clang
CXX = /usr/bin/clang++
OPT = /opt/homebrew/opt/llvm@17/bin/opt
LLC = /usr/bin/llc
LLVM_CONFIG = llvm-config
CCFLAGS = -Wall -Werror -pedantic-errors
CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags) $(CCFLAGS) -fPIC -std=c++20
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --system-libs --libs all)

all: $(TESTFILE_OPT_LL) $(BIN) $(BIN_OPT)

all-test: $(TESTFILE_OPT_LL) $(BIN) $(BIN_OPT)

$(TESTFILE_OPT_LL): $(TESTFILE_LL) $(OPTIMIZER)
	$(OPT) -load-pass-plugin=./$(OPTIMIZER) -passes=custom-loopfusion -S $< -o $@

$(TESTFILE_LL): $(TESTFILE_SRC)
	$(CC) -O0 -Xclang -disable-O0-optnone -emit-llvm -S $< -o $@ -DARRAY_SIZE=$(ARRAY_SIZE)
	$(OPT) -passes=mem2reg -S $@ -o $@

$(TESTFILE_OBJ): $(TESTFILE_LL)
	$(CC) -c $< -o $@

$(TESTFILE_OPT_OBJ): $(TESTFILE_OPT_LL)
	$(CC) -c $< -o $@

$(OPTIMIZER): $(LIB_OBJ)
	$(CXX) -shared $^ -o $@ $(LDFLAGS)

$(LIB_OBJ): $(LIB_SRC) $(LIB_INCLUDE) | $(LIB_DIR) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_SRC)
	$(CC) $(CCFLAGS) -c $< -o $@ -DARRAY_SIZE=$(ARRAY_SIZE)

$(BIN): $(MAIN_OBJ) $(TESTFILE_OBJ) | $(BIN_DIR)
	$(CC) -o $@ $^

$(BIN_OPT): $(MAIN_OBJ) $(TESTFILE_OPT_OBJ) | $(BIN_DIR)
	$(CC) -o $@ $^

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

.PHONY: clean clean-test
clean:
	$(RM) $(BIN_DIR)/* $(OBJ_DIR)/* $(OPTIMIZER) $(TEST_DIR)/*.ll

clean-test:
	$(RM) $(MAIN_OBJ) $(TESTFILE_OBJ) $(TESTFILE_OPT_OBJ) $(TESTFILE_LL) $(TESTFILE_OPT_LL)