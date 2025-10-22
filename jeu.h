#ifndef JEU_H
#define JEU_H

#include "plateau.h"

// === Structure utilisée pour jouer/annuler un coup ===
typedef struct {
    int case_depart;
    int couleur;
    int mode_transp;
    int joueur;
    int captures;
    Plateau* derniere;
    int R[16], B[16], T[16]; // état du plateau avant le coup
} Coup;

// couleur: 1=R, 2=B, 3=T ; mode_transp: 1 = comme R, 2 = comme B (ignoré sinon)
Plateau* distribuer(Plateau* depart, int couleur, int joueur, int mode_transp);

// capture à partir de la dernière case semée, en remontant tant que total ∈ {2,3}
int capturer(Plateau* plateau, Plateau* derniere, int nb_cases);

// nouvelles fonctions d’IA rapides
Plateau* jouer_coup(Plateau* plateau, Coup* c, int nb_cases);
void annuler_coup(Plateau* plateau, Coup* c, int nb_cases);

#endif
