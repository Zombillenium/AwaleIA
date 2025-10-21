#include <stdio.h>
#include <stdlib.h>

// Définition de la structure Plateau
typedef struct Plateau {
    int caseN;
    int R;
    int B;
    int T;
    struct Plateau* caseSuiv;  // pointeur vers la case suivante
} Plateau;

// Fonction pour créer un plateau circulaire de nb_cases cases
Plateau* creer_plateau(int nb_cases) {
    Plateau* premiere = NULL;
    Plateau* precedente = NULL;

    for (int i = 0; i < nb_cases; i++) {
        Plateau* nouvelle = malloc(sizeof(Plateau));
        if (!nouvelle) {
            printf("Erreur d'allocation mémoire.\n");
            exit(1);
        }

        nouvelle->caseN = i + 1;
        nouvelle->R = 4;
        nouvelle->B = 4;
        nouvelle->T = 4;
        nouvelle->caseSuiv = NULL;

        if (precedente != NULL) {
            precedente->caseSuiv = nouvelle;
        } else {
            premiere = nouvelle;
        }

        precedente = nouvelle;
    }

    // relier la dernière case à la première pour former une boucle
    precedente->caseSuiv = premiere;

    return premiere;
}

// Fonction pour afficher le plateau
void afficher_plateau(Plateau* plateau, int nb_cases) {
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

// Fonction principale
int main() {
    int nb_cases = 16;
    Plateau* plateau = creer_plateau(nb_cases);
    char* joueurs[2] = {"J1", "J2"};
    int tour = 0;

    printf("=== Début du jeu ===\n");
    printf("Plateau créé avec %d cases (4R, 4B, 4T chacune).\n", nb_cases);
    printf("Les joueurs vont jouer à tour de rôle.\n");

    afficher_plateau(plateau, nb_cases);

    // Boucle de jeu (ex: 6 tours pour tester)
    for (int i = 0; i < 6; i++) {
        printf("%s, appuie sur Entrée pour jouer ton tour... ", joueurs[tour]);
        getchar();  // attend que l’utilisateur appuie sur Entrée

        printf("%s a joué (aucune action pour l’instant).\n", joueurs[tour]);
        afficher_plateau(plateau, nb_cases);

        printf("----------------------------------------\n");

        tour = 1 - tour;  // alterner entre 0 et 1
    }

    printf("=== Fin de la partie (temporaire) ===\n");

    return 0;
}
