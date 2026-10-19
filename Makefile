# Genuinely just opus 5.5 for this


CXX      := clang++
WARN     := -Wall -Wextra -Werror -Wunreachable-code
BASE     := -std=c++20 $(WARN) -Iinclude -MMD -MP

MODE ?= debug
ifeq ($(MODE),release)
  CXXFLAGS := $(BASE) -O2 -DNDEBUG
  LDFLAGS  :=
else
  CXXFLAGS := $(BASE) -O2 -g3 -fsanitize=address,undefined
  LDFLAGS  := -fsanitize=address,undefined
endif

ifeq ($(MODE),release)
ifneq ($(filter test,$(MAKECMDGOALS)),)
  $(error Tests rely on assert, which release mode disables. Use MODE=debug)
endif
endif

BUILD_DIR := build/$(MODE)

SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=$(BUILD_DIR)/obj/%.o)
LIB_OBJ  := $(filter-out $(BUILD_DIR)/obj/main.o,$(OBJ))
APP      := $(BUILD_DIR)/engine

TEST_SRC := $(wildcard tests/*.cpp)
TEST_BIN := $(TEST_SRC:tests/%.cpp=$(BUILD_DIR)/tests/%)

.PHONY: all run test clean
all: $(APP)

$(APP): $(OBJ)
	@mkdir -p $(@D)
	$(CXX) $^ $(LDFLAGS) -o $@

$(BUILD_DIR)/obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%: tests/%.cpp $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(LIB_OBJ) $(LDFLAGS) -o $@

run: $(APP)
	./$(APP)

test: $(TEST_BIN)
	@for t in $(TEST_BIN); do echo "== $$t"; ./$$t || exit 1; done

clean:
	rm -rf build

-include $(OBJ:.o=.d) $(TEST_BIN:=.d)


# Commands btw
# make to build
# make run to build and run 
# make test builds and runs every test
# make MODE=release builds an optimised version without sanitizers
# make clean wipes it all