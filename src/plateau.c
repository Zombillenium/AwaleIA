#include <stdio.h>
#include <stdlib.h>
#include "plateau.h"

void liberer_plateau(Plateau* plateau) {
    if (!plateau) return;
    Plateau* p = plateau;
    Plateau* suivant;
    for (int i = 0; i < nb_cases; i++) {
        suivant = p->caseSuiv;
        free(p);
        p = suivant;
    }
}

Plateau* creer_plateau() {
    Plateau* premiere = NULL;
    Plateau* precedente = NULL;

    for (int i = 0; i < nb_cases; i++) {
        Plateau* nouvelle = malloc(sizeof(Plateau));
        if (!nouvelle) { printf("Erreur d'allocation mémoire.\n"); exit(1); }

        nouvelle->caseN = i + 1;
        nouvelle->R = 2;
        nouvelle->B = 2;
        nouvelle->T = 2;
        nouvelle->caseSuiv = NULL;

        if (precedente) precedente->caseSuiv = nouvelle;
        else premiere = nouvelle;
        precedente = nouvelle;
    }

    precedente->caseSuiv = premiere;
    return premiere;
}

void afficher_plateau(Plateau* plateau) {
    Plateau* p = plateau;
    printf("\n=== État du plateau ===\n");
    printf("Case\tR\tB\tT\n");
    printf("-------------------------\n");
    for (int i = 0; i < nb_cases; i++) {
        printf("%d\t%d\t%d\t%d\n", p->caseN, p->R, p->B, p->T);
        p = p->caseSuiv;
    }
    printf("-------------------------\n\n");
}

Plateau* trouver_case(Plateau* plateau, int num) {
    Plateau* p = plateau;
    do {
        if (p->caseN == num) return p;
        p = p->caseSuiv;
    } while (p != plateau);
    return NULL;
}

Plateau* case_precedente(Plateau* plateau, Plateau* cible) {
    Plateau* p = plateau;
    while (p->caseSuiv != cible) {
        p = p->caseSuiv;
    }
    return p;
}

inline int case_du_joueur(int caseN, int joueur) {
    return (joueur == 1) ? (caseN & 1) : !(caseN & 1);
}


int total_graines(Plateau* plateau) {
    int total = 0;
    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        total += p->R + p->B + p->T;
        p = p->caseSuiv;
    }
    return total;
}

int total_graines_joueur(Plateau* plateau, int joueur) {
    int total = 0;
    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        if (case_du_joueur(p->caseN, joueur))
            total += p->R + p->B + p->T;
        p = p->caseSuiv;
    }
    return total;
}
