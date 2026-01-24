# === Variables principales ===

CC       := gcc
CC_WIN   := x86_64-w64-mingw32-gcc

NAME     := tracker_modulaire
NAME_WIN := $(NAME).exe

BUILD    := build
BIN      := bin

INCLUDES := -Iinclude

CFLAGS_COMMON := -Wall -Wextra -Werror -Iinclude
CFLAGS_C99    := -std=c99
CFLAGS_LINUX  := -pthread
CFLAGS_WIN    :=

LDFLAGS  := -lm

# === Sources ===
# On exclut le fichier de test des builds normal/win
SRC := $(filter-out src/test_hunt_rules.c, $(wildcard src/*.c))

OBJ      := $(patsubst %.c, $(BUILD)/%.o, $(SRC))
OBJ_WIN  := $(patsubst %.c, $(BUILD)/win/%.o, $(SRC))

# === Phony ===
.PHONY: all win clean debug release run test

# === Build Linux ===

all: $(BIN)/$(NAME)

$(BIN)/$(NAME): $(OBJ)
	@mkdir -p $(BIN)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C99) $(CFLAGS_COMMON) $(CFLAGS_LINUX) $(INCLUDES) -c $< -o $@

# === Build Windows (.exe) ===

win: $(BIN)/$(NAME_WIN)

$(BIN)/$(NAME_WIN): $(OBJ_WIN)
	@echo "====> Compilation pour Windows (.exe)"
	@mkdir -p $(BIN)
	$(CC_WIN) $^ -o $@ $(LDFLAGS)
	@echo "====> Fichier généré : $(BIN)/$(NAME_WIN)"

$(BUILD)/win/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC_WIN) $(CFLAGS_C99) $(CFLAGS_COMMON) $(CFLAGS_WIN) $(INCLUDES) -c $< -o $@

# === Règles utilitaires ===

run: all
	./$(BIN)/$(NAME)

debug: CFLAGS_COMMON += -g -DDEBUG
debug: clean all

release: CFLAGS_COMMON += -O2
release: clean all

clean:
	rm -rf $(BUILD) $(BIN)

# === Tests ===

TEST_NAME := test_hunt_rules
TEST_SRC  := src/test_hunt_rules.c src/hunt_rules.c

test:
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS_C99) $(CFLAGS_COMMON) $(CFLAGS_LINUX) $(INCLUDES) $(TEST_SRC) -o $(BIN)/$(TEST_NAME) $(LDFLAGS)
	./$(BIN)/$(TEST_NAME) tests/hunt_rules_cases.txt
