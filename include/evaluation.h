#ifndef EVALUATION_H
#define EVALUATION_H

#include "plateau.h"

// Fonction d'évaluation du plateau
int evaluer_plateau(Plateau* plateau, int joueur);
int evaluer_plateau_famine(Plateau* plateau, int joueur);

// Nouvelles fonctions d'évaluation avancées
int evaluer_plateau_avance(Plateau* plateau, int joueur);
int evaluer_plateau_famine_avance(Plateau* plateau, int joueur);

// Fonctions utilitaires pour patterns
int detecter_menace_famine(Plateau* plateau, int joueur);
int compter_cases_critiques(Plateau* plateau, int joueur);
float calculer_phase_jeu(Plateau* plateau);

#endif
