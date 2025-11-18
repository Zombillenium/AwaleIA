#ifndef TABLETRANSPO_H
#define TABLETRANSPO_H

#include "plateau.h"

#define HASH_SIZE  50000
#define MAX_CASES  nb_cases   // = 16, défini dans plateau.h
#define MAX_GRAINES 96

typedef struct {
    unsigned long long key;
    int depth;
    int score;
    int valid;
} Entry;

// --- Fonctions principales ---
unsigned long long hash_plateau(Plateau* plateau);
int chercher_transpo(int joueur, unsigned long long key, int profondeur, int* score_out);
void ajouter_transpo(int joueur, unsigned long long key, int profondeur, int score);
void vider_transpo_joueur(int joueur);
void vider_toutes_transpos();
void initialiser_transpo_lock();

// --- Initialisation du Zobrist ---
void initialiser_zobrist(void);

#endif
