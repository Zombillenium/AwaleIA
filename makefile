# === Noms des exécutables ===
EXEC_MAIN    = main
EXEC_BATTLE  = main_battle
EXEC_MQTT    = main_mqtt

# === Compilateur ===
CC = gcc

# === Options de compilation communes ===
CFLAGS = -Wall -Wextra -std=c11 -O3 -march=native -flto -funroll-loops -DNDEBUG -pipe -fopenmp
LDFLAGS = -lpaho-mqtt3cs -lpthread -lssl -lcrypto   # ⚙️ version synchrone avec SSL

# === Fichiers sources communs ===
SRC_COMMON = plateau.c jeu.c evaluation.c ia.c tabletranspo.c

# === Fichiers spécifiques ===
SRC_MAIN    = main.c $(SRC_COMMON)
SRC_BATTLE  = main_battle.c $(SRC_COMMON)
SRC_MQTT    = main_MQTT.c $(SRC_COMMON)

# === Objets ===
OBJ_MAIN    = $(SRC_MAIN:.c=.o)
OBJ_BATTLE  = $(SRC_BATTLE:.c=.o)
OBJ_MQTT    = $(SRC_MQTT:.c=.o)

# === Règle par défaut ===
all: $(EXEC_MAIN) $(EXEC_BATTLE) $(EXEC_MQTT)

# === Compilation de main ===
$(EXEC_MAIN): $(OBJ_MAIN)
	@echo "🔧 Édition des liens pour $(EXEC_MAIN)..."
	$(CC) $(OBJ_MAIN) -o $(EXEC_MAIN) $(CFLAGS)
	@echo "✅ Compilation terminée : $(EXEC_MAIN)"

# === Compilation de main_battle ===
$(EXEC_BATTLE): $(OBJ_BATTLE)
	@echo "🔧 Édition des liens pour $(EXEC_BATTLE)..."
	$(CC) $(OBJ_BATTLE) -o $(EXEC_BATTLE) $(CFLAGS)
	@echo "✅ Compilation terminée : $(EXEC_BATTLE)"

# === Compilation de main_mqtt ===
$(EXEC_MQTT): $(OBJ_MQTT)
	@echo "🔧 Édition des liens pour $(EXEC_MQTT)..."
	$(CC) $(OBJ_MQTT) -o $(EXEC_MQTT) $(CFLAGS) $(LDFLAGS)
	@echo "✅ Compilation terminée : $(EXEC_MQTT)"

# === Compilation générique ===
%.o: %.c
	@echo "🧩 Compilation de $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# === Nettoyage ===
clean:
	@echo "🧹 Nettoyage..."
	rm -f *.o

mrproper: clean
	rm -f $(EXEC_MAIN) $(EXEC_BATTLE) $(EXEC_MQTT)

run-mqtt: $(EXEC_MQTT)
	@echo "🌐 Lancement de $(EXEC_MQTT)..."
	./$(EXEC_MQTT) 0
