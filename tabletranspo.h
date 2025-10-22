#ifndef TABLETRANSPO_H
#define TABLETRANSPO_H

#include "plateau.h"

#define HASH_SIZE 200003  // grand nombre premier, ajustable

typedef struct {
    unsigned long long key;
    int depth;
    int score;
    int valid;
} Entry;

extern Entry transpo_table[HASH_SIZE];

// Génère une clé unique pour un plateau
unsigned long long hash_plateau(Plateau* plateau, int nb_cases);

// Cherche une entrée dans la table
int chercher_transpo(unsigned long long key, int profondeur, int* score_out);

// Ajoute une entrée
void ajouter_transpo(unsigned long long key, int profondeur, int score);

// Réinitialise la table (utile avant une partie)
void vider_transpo();

#endif
