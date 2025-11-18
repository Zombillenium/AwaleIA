#ifndef JEU_H
#define JEU_H

#include "plateau.h"

typedef struct {
    Plateau* plateau;
    int scores[2];
    int tour;      // 0 = joueur 0, 1 = joueur 1
    // plus de nb_cases ici, on utilise la macro nb_cases définie dans plateau.h
} Partie;

// === Structure utilisée pour jouer/annuler un coup ===
typedef struct {
    int case_depart;
    int couleur;
    int mode_transp;
    int joueur;
    int captures;
    Plateau* derniere;
    int R[nb_cases], B[nb_cases], T[nb_cases]; // état du plateau avant le coup
} Coup;

typedef struct {
    int caseN;
    int couleur;  // 1 = R, 2 = B, 3 = T
    int mode;     // pour T : 1 = TR, 2 = TB ; sinon 0
} Move;

#define SCORE_VICTOIRE  100000
#define SCORE_DEFAITE  -100000


// couleur: 1=R, 2=B, 3=T ; mode_transp: 1 = comme R, 2 = comme B (ignoré sinon)
Plateau* distribuer(Plateau* depart, int couleur, int joueur, int mode_transp);

// capture à partir de la dernière case semée, en remontant tant que total ∈ {2,3}
int capturer(Plateau* plateau, Plateau* derniere);

// nouvelles fonctions d’IA rapides
Plateau* jouer_coup(Plateau* plateau, Coup* c);
void annuler_coup(Plateau* plateau, Coup* c);

int verifier_conditions_fin(Partie* partie);
int evaluer_fin_de_partie(Plateau* plateau, int joueur);

#endif
