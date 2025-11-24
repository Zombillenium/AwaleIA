#ifndef IA_H
#define IA_H

#include "plateau.h"
#include "tabletranspo.h"
#include "jeu.h"

// Forward declaration pour éviter dépendance circulaire
struct ParametresEvaluation;
typedef struct ParametresEvaluation ParametresEvaluation;

// --- Fonctions principales ---
void meilleur_coup(Plateau* plateau, int joueur); // si tu ne l'utilises pas, ce n'est pas grave
int minimax(Plateau* plateau, int joueur, int profondeur, int alpha, int beta, int maximisant);
int minimax_tune(Plateau* plateau, int joueur, int profondeur, int alpha, int beta, int maximisant, ParametresEvaluation* params);
int choisir_meilleur_coup(Partie* partie, int joueur, int* best_case, int* best_color, int* best_mode);
void definir_type_ia(int joueur, int type_ia); // type_ia: 1 = classique, 2 = famine

// --- Fonctions utilitaires ---
Plateau* copier_plateau(Plateau* original);
void initialiser_killer_moves();

// --- Heuristiques et killer moves ---
void enregistrer_killer_move(int profondeur, Coup coup);
void tester_killer_moves(Plateau* plateau, int joueur, int joueur_actuel,
                         int profondeur,
                         int* meilleur_score, int* alpha, int* beta, int maximisant);

// --- Fonctions d’aide au tri ---
void trier_cases_par_valeur(Plateau* plateau, int* ordre, int* valeurs);

#endif
