# === Noms des exécutables ===
EXEC_MAIN   = main
EXEC_BATTLE = main_battle

# === Compilateur ===
CC = gcc

# === Options de compilation communes ===
CFLAGS = -Wall -Wextra -std=c11 -O3 -march=native -flto -funroll-loops -DNDEBUG -pipe -fopenmp

# === Fichiers sources communs ===
SRC_COMMON = plateau.c jeu.c evaluation.c ia.c tabletranspo.c

# === Fichiers spécifiques ===
SRC_MAIN   = main.c $(SRC_COMMON)
SRC_BATTLE = main_battle.c $(SRC_COMMON)

# === Objets ===
OBJ_MAIN   = $(SRC_MAIN:.c=.o)
OBJ_BATTLE = $(SRC_BATTLE:.c=.o)

# === Règle par défaut ===
all: $(EXEC_MAIN) $(EXEC_BATTLE)

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

# === Règle générique pour compiler les .c en .o ===
%.o: %.c
	@echo "🧩 Compilation de $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# === Nettoyage des fichiers objets ===
clean:
	@echo "🧹 Nettoyage des fichiers objets..."
	rm -f *.o

# === Nettoyage complet (objets + exécutables) ===
mrproper: clean
	@echo "🧽 Suppression des exécutables..."
	rm -f $(EXEC_MAIN) $(EXEC_BATTLE)

# === Exécution rapide ===
run-main: $(EXEC_MAIN)
	@echo "🚀 Lancement de $(EXEC_MAIN)..."
	./$(EXEC_MAIN)

run-battle: $(EXEC_BATTLE)
	@echo "🎮 Lancement de $(EXEC_BATTLE)..."
	./$(EXEC_BATTLE)
