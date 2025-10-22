#ifndef IA_H
#define IA_H

#include "plateau.h"
#include "tabletranspo.h"


void meilleur_coup(Plateau* plateau, int joueur);
Plateau* copier_plateau(Plateau* original, int nb_cases);
int minimax(Plateau* plateau, int joueur, int profondeur, int alpha, int beta, int maximisant);
#endif
