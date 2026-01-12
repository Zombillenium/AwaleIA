# === Dossiers ===
SRC_DIR     = src
INC_DIR     = include
BUILD_DIR   = build
BIN_DIR     = bin

# === Noms des exécutables ===
EXEC_MAIN    = $(BIN_DIR)/main
EXEC_BATTLE  = $(BIN_DIR)/main_battle
EXEC_MQTT    = $(BIN_DIR)/main_mqtt
EXEC_TUNING  = $(BIN_DIR)/main_tuning
EXEC_EVAL    = $(BIN_DIR)/main_eval

# === Compilateur ===
CC = gcc

# === Options de compilation ===
CFLAGS = -Wall -Wextra -std=c11 -O3 -funroll-loops -DNDEBUG -pipe -fopenmp -I$(INC_DIR)
LDFLAGS = -lpaho-mqtt3cs -lpthread -lssl -lcrypto

# === Fichiers sources ===
SRC_COMMON  = plateau.c jeu.c evaluation.c ia.c tabletranspo.c tuning.c

SRC_MAIN    = main.c $(SRC_COMMON)
SRC_BATTLE  = main_battle.c $(SRC_COMMON)
SRC_MQTT    = main_MQTT.c $(SRC_COMMON)
SRC_TUNING  = main_tuning.c $(SRC_COMMON)
SRC_EVAL    = main_eval.c $(SRC_COMMON)

# === Génération automatique des .o ===
OBJ_MAIN    = $(addprefix $(BUILD_DIR)/, $(SRC_MAIN:.c=.o))
OBJ_BATTLE  = $(addprefix $(BUILD_DIR)/, $(SRC_BATTLE:.c=.o))
OBJ_MQTT    = $(addprefix $(BUILD_DIR)/, $(SRC_MQTT:.c=.o))
OBJ_TUNING  = $(addprefix $(BUILD_DIR)/, $(SRC_TUNING:.c=.o))
OBJ_EVAL    = $(addprefix $(BUILD_DIR)/, $(SRC_EVAL:.c=.o))

# === Règle par défaut (sans MQTT) ===
all: $(EXEC_MAIN) $(EXEC_BATTLE) $(EXEC_TUNING) $(EXEC_EVAL)

# === Règle avec MQTT (nécessite paho-mqtt) ===
all-mqtt: $(EXEC_MAIN) $(EXEC_BATTLE) $(EXEC_MQTT) $(EXEC_TUNING) $(EXEC_EVAL)

# === Compilation des exécutables ===
$(EXEC_MAIN): $(OBJ_MAIN)
	$(CC) $^ -o $@ $(CFLAGS)

$(EXEC_BATTLE): $(OBJ_BATTLE)
	$(CC) $^ -o $@ $(CFLAGS)

$(EXEC_MQTT): $(OBJ_MQTT)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

$(EXEC_TUNING): $(OBJ_TUNING)
	$(CC) $^ -o $@ $(CFLAGS)

$(EXEC_EVAL): $(OBJ_EVAL)
	$(CC) $^ -o $@ $(CFLAGS)

# === Compilation générique dans build/ ===
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# === Création des répertoires ===
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

# === Nettoyage ===
clean:
	rm -rf $(BUILD_DIR)

mrproper: clean
	rm -rf $(BIN_DIR)

run-mqtt: $(EXEC_MQTT)
	./$(EXEC_MQTT) 0
