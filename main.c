#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "plateau.h"
#include "jeu.h"

int main() {
    int nb_cases = 16;
    Plateau* plateau = creer_plateau(nb_cases);
    char* joueurs[2] = {"J1", "J2"};
    int scores[2] = {0, 0};
    int tour = 0;

    printf("=== Début du jeu Awalé modifié ===\n");
    afficher_plateau(plateau, nb_cases);

    while (1) {
        printf("Tour de %s\n", joueurs[tour]);
        char entree[10];
        int num_case = 0, couleur = 0, mode_transp = 0;

        printf("Entre ton coup (ex : 16B, 8R, 3TB, 5TR) : ");
        if (scanf("%9s", entree) != 1) break;

        int i = 0; while (isdigit(entree[i])) i++;
        num_case = atoi(entree);

        if (entree[i] == 'R' || entree[i] == 'r') couleur = 1;
        else if (entree[i] == 'B' || entree[i] == 'b') couleur = 2;
        else if (entree[i] == 'T' || entree[i] == 't') {
            couleur = 3; i++;
            if (entree[i] == 'R' || entree[i] == 'r') mode_transp = 1;
            else if (entree[i] == 'B' || entree[i] == 'b') mode_transp = 2;
            else { printf("Transparente sans mode (R/B) invalide.\n"); continue; }
        } else {
            printf("Couleur invalide.\n"); continue;
        }

        Plateau* c = trouver_case(plateau, num_case);
        if (!c || !case_du_joueur(num_case, tour + 1)) {
            printf("Case invalide pour ce joueur.\n"); continue;
        }

        Plateau* derniere = distribuer(c, couleur, tour + 1, mode_transp);
        int captures = capturer(plateau, derniere);
        scores[tour] += captures;

        printf("%s a capturé %d graines.\n", joueurs[tour], captures);
        afficher_plateau(plateau, nb_cases);
        printf("Scores -> J1: %d | J2: %d\n", scores[0], scores[1]);
        printf("----------------------------------------\n");

        tour = 1 - tour;
    }
    return 0;
}
