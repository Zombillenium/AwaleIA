#ifndef PLATEAU_H
#define PLATEAU_H

#define nb_cases 16

typedef struct Plateau {
    int caseN;
    int R;
    int B;
    int T;
    struct Plateau* caseSuiv;
} Plateau;

Plateau* creer_plateau();
void afficher_plateau(Plateau* plateau);
Plateau* trouver_case(Plateau* plateau, int num);
Plateau* case_precedente(Plateau* plateau, Plateau* cible);
int case_du_joueur(int caseN, int joueur);
void liberer_plateau(Plateau* plateau);
int total_graines_joueur(Plateau* plateau, int joueur);
int total_graines(Plateau* plateau);


#endif
