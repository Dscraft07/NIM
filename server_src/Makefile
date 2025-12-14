# ============================================
# Makefile pro Nim Game Server
# ============================================

# Kompilator a flagy
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread

# Adresare
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Vystupni soubor
TARGET = nim_server

# Zdrojove soubory
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

# ============================================
# Pravidla
# ============================================

.PHONY: all clean debug release

# Vychozi cil - release build
all: release

# Release build (s optimalizacemi)
release: CFLAGS += -O2
release: $(TARGET)

# Debug build (pro ladeni)
debug: CFLAGS += -g -DDEBUG -O0
debug: $(TARGET)

# Sestaveni vysledneho programu
$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Kompilace zdrojovych souboru
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -MP -c $< -o $@

# Vytvoreni adresare pro build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Cisteni
clean:
	rm -rf $(BUILD_DIR) $(TARGET) *.log core

# Zahrn zavislosti
-include $(DEPS)

# ============================================
# Pomocne cile
# ============================================

# Spusteni s vychozimi parametry
run: release
	./$(TARGET)

# Spusteni s vlastnimi parametry
run-custom: release
	./$(TARGET) -a 0.0.0.0 -p 10000 -c 50 -r 10

# Kontrola memory leaks pomoci valgrind
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Formatovani kodu (vyzaduje clang-format)
format:
	find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | xargs clang-format -i

# Napoveda
help:
	@echo "Dostupne cile:"
	@echo "  all (release) - Sestavi release verzi"
	@echo "  debug         - Sestavi debug verzi"
	@echo "  clean         - Smaze sestavene soubory"
	@echo "  run           - Spusti server s vychozimi parametry"
	@echo "  run-custom    - Spusti server s vlastnimi parametry"
	@echo "  valgrind      - Spusti s kontrolou pameti"
	@echo "  format        - Zformatuje zdrojovy kod"
	@echo "  help          - Zobrazi tuto napovedu"

