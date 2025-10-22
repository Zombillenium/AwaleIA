#ifndef PLATEAU_H
#define PLATEAU_H

typedef struct Plateau {
    int caseN;
    int R;
    int B;
    int T;
    struct Plateau* caseSuiv;
} Plateau;

// Fonctions de gestion du plateau
Plateau* creer_plateau(int nb_cases);
void afficher_plateau(Plateau* plateau, int nb_cases);
Plateau* trouver_case(Plateau* plateau, int num);
Plateau* case_precedente(Plateau* plateau, Plateau* cible);
int case_du_joueur(int caseN, int joueur);

#endif
