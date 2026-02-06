# === Variables principales ===

CC       := gcc
CC_WIN   := x86_64-w64-mingw32-gcc

NAME     := tracker_loot
NAME_WIN := $(NAME).exe

BUILD    := build
BIN      := bin

INCLUDES := -Iinclude

CFLAGS_COMMON := -Wall -Wextra -Werror -Iinclude
CFLAGS_C99    := -std=c99
CFLAGS_LINUX  :=
CFLAGS_WIN    :=

# === Link flags selon OS ===
LDFLAGS_LINUX := -lX11 -lm
LDFLAGS_WIN   := -luser32 -lgdi32

# === Sources ===
SRC := $(wildcard src/*.c)

OBJ      := $(patsubst %.c, $(BUILD)/%.o, $(SRC))
OBJ_WIN  := $(patsubst %.c, $(BUILD)/win/%.o, $(SRC))

# === Phony ===
.PHONY: all win clean debug release run

# === Build Linux ===

all: $(BIN)/$(NAME)

$(BIN)/$(NAME): $(OBJ)
	@mkdir -p $(BIN)
	$(CC) $^ -o $@ $(LDFLAGS_LINUX)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C99) $(CFLAGS_COMMON) $(CFLAGS_LINUX) $(INCLUDES) -c $< -o $@

# === Build Windows (.exe) ===

win: $(BIN)/$(NAME_WIN)

$(BIN)/$(NAME_WIN): $(OBJ_WIN)
	@echo "====> Compilation pour Windows (.exe)"
	@mkdir -p $(BIN)
	$(CC_WIN) $^ -o $@ $(LDFLAGS_WIN)
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
