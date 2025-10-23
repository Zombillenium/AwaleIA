#ifndef IA_H
#define IA_H

#include "plateau.h"
#include "tabletranspo.h"
#include "jeu.h"


// --- Fonctions principales ---
void meilleur_coup(Plateau* plateau, int joueur);
int minimax(Plateau* plateau, int joueur, int profondeur, int alpha, int beta, int maximisant);

// --- Fonctions utilitaires ---
Plateau* copier_plateau(Plateau* original, int nb_cases);
void initialiser_killer_moves();

// --- Heuristiques et killer moves ---
void enregistrer_killer_move(int profondeur, Coup coup);
void tester_killer_moves(Plateau* plateau, int joueur, int joueur_actuel,
                         int profondeur, int nb_cases,
                         int* meilleur_score, int* alpha, int* beta, int maximisant);

// --- Fonctions d’aide au tri ---
void trier_cases_par_valeur(Plateau* plateau, int nb_cases, int* ordre, int* valeurs);

#endif
