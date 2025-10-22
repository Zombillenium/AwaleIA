# === Nom de l'exécutable ===
EXEC = main

# === Compilateur ===
CC = gcc

# === Options de compilation ===
# -O3            : optimisation maximale
# -march=native  : tire parti des instructions CPU de ta machine
# -flto          : optimisation à l’édition de liens (LTO)
# -funroll-loops : déroule les boucles (utile pour ton IA récursive)
# -DNDEBUG       : désactive les assertions pour gagner du temps
# -pipe          : accélère la compilation
CFLAGS = -Wall -Wextra -std=c11 -O3 -march=native -flto -funroll-loops -DNDEBUG -pipe

# === Fichiers sources ===
SRC = main.c plateau.c jeu.c evaluation.c ia.c tabletranspo.c

# === Objets générés automatiquement ===
OBJ = $(SRC:.c=.o)

# === Règle par défaut ===
all: release

# === Mode release (optimisé) ===
release: CFLAGS += -s  # supprime les symboles de debug pour un exécutable plus léger
release: $(EXEC)

# === Mode debug (avec infos gdb et sans optimisations) ===
debug: CFLAGS = -Wall -Wextra -std=c11 -Og -g
debug: clean $(EXEC)

# === Compilation de l'exécutable ===
$(EXEC): $(OBJ)
	@echo "🔧 Édition des liens..."
	$(CC) $(OBJ) -o $(EXEC) $(CFLAGS)
	@echo "✅ Compilation terminée : $(EXEC)"

# === Règle générique pour compiler les .c en .o ===
%.o: %.c
	@echo "🧩 Compilation de $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# === Nettoyage des fichiers objets ===
clean:
	@echo "🧹 Nettoyage des fichiers objets..."
	rm -f $(OBJ)

# === Nettoyage complet (objets + exécutable) ===
mrproper: clean
	@echo "🧽 Suppression de l’exécutable..."
	rm -f $(EXEC)

# === Raccourci pour exécuter directement le programme ===
run: $(EXEC)
	@echo "🚀 Lancement de $(EXEC)..."
	./$(EXEC)
