# === Nom de l'exécutable ===
EXEC = awale

# === Compilateur et options ===
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# === Fichiers sources et objets ===
SRC = main.c plateau.c jeu.c
OBJ = $(SRC:.c=.o)

# === Règle par défaut ===
all: $(EXEC)

# === Compilation de l'exécutable ===
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC)

# === Règle générique pour compiler les .c en .o ===
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# === Nettoyage des fichiers compilés ===
clean:
	rm -f $(OBJ)

# === Nettoyage complet (objets + exécutable) ===
mrproper: clean
	rm -f $(EXEC)

# === Règle pour relancer le programme ===
run: $(EXEC)
	./$(EXEC)
