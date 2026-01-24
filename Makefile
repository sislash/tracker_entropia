# === Variables principales ===

CC       := gcc
CC_WIN   := x86_64-w64-mingw32-gcc

NAME     := tracker_modulaire
NAME_WIN := $(NAME).exe

BUILD    := build
BIN      := bin

CFLAGS   := -std=c99 -Wall -Wextra -Werror -Iinclude -pthread
LDFLAGS  := -lm

# === Dossier source ===

# Projet "tracker_modulaire" : headers dans include/, sources dans src/
INCLUDES := -Iinclude

MODULES  := src

# === où se trouve le main() ===

# On exclut le fichier de test des builds normal/win
SRC := $(filter-out src/test_hunt_rules.c, $(wildcard src/*.c))

OBJ := $(patsubst %.c, $(BUILD)/%.o, $(SRC))

# === Règles ===

.PHONY: all win clean debug release run test

all: $(BIN)/$(NAME)

$(BIN)/$(NAME): $(OBJ)
	@mkdir -p $(BIN)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compilation de chaque .c en .o dans le dossier build/
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# === Compilation Windows (.exe) ===

win: CFLAGS += -std=c11 -O2
win: clean
	@echo "====> Compilation pour Windows (.exe)"
	@mkdir -p $(BIN)
	@$(CC_WIN) $(CFLAGS) $(INCLUDES) $(SRC) -o $(BIN)/$(NAME_WIN) -lm $(LDFLAGS)
	@echo "====> Fichier généré : $(BIN)/$(NAME_WIN)"

# === Autres règles ===

run: all
	./$(BIN)/$(NAME)

debug: CFLAGS += -g -DDEBUG
debug: clean all

release: CFLAGS += -O2
release: clean all

clean:
	rm -rf $(BUILD) $(BIN)

# === Tests ===

TEST_NAME := test_hunt_rules
TEST_SRC  := src/test_hunt_rules.c src/hunt_rules.c

test:
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_SRC) -o $(BIN)/$(TEST_NAME) $(LDFLAGS)
	./$(BIN)/$(TEST_NAME) tests/hunt_rules_cases.txt
