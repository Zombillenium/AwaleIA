#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// === Structures ===
typedef struct Plateau {
    int caseN;
    int R;
    int B;
    int T;
    struct Plateau* caseSuiv;
} Plateau;

// === Fonctions utilitaires ===
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
        nouvelle->R = 2;
        nouvelle->B = 2;
        nouvelle->T = 2;
        nouvelle->caseSuiv = NULL;

        if (precedente)
            precedente->caseSuiv = nouvelle;
        else
            premiere = nouvelle;

        precedente = nouvelle;
    }

    precedente->caseSuiv = premiere;
    return premiere;
}

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

Plateau* trouver_case(Plateau* plateau, int num) {
    Plateau* p = plateau;
    do {
        if (p->caseN == num) return p;
        p = p->caseSuiv;
    } while (p != plateau);
    return NULL;
}

Plateau* case_precedente(Plateau* plateau, Plateau* cible, int nb_cases) {
    Plateau* p = plateau;
    while (p->caseSuiv != cible) {
        p = p->caseSuiv;
    }
    return p;
}

int case_du_joueur(int caseN, int joueur) {
    return (joueur == 1) ? (caseN % 2 == 1) : (caseN % 2 == 0);
}

int capturer(Plateau* plateau, Plateau* derniere, int nb_cases, int joueur) {
    int total_capture = 0;
    Plateau* courante = derniere;

    while (1) {
        int total = courante->R + courante->B + courante->T;
        if (total == 2 || total == 3) {
            total_capture += total;
            courante->R = courante->B = courante->T = 0;
            courante = case_precedente(plateau, courante, nb_cases);
        } else {
            break;
        }
    }

    return total_capture;
}

Plateau* distribuer(Plateau* plateau, Plateau* depart, int couleur, int joueur, int nb_cases, int mode_transp) {
    Plateau* courante = depart;
    int graines = 0;

    if (couleur == 1) {
        graines = depart->R;
        depart->R = 0;
    } else if (couleur == 2) {
        graines = depart->B;
        depart->B = 0;
    } else if (couleur == 3) {
        graines = depart->T;
        depart->T = 0;
    }

    Plateau* derniere = NULL;

    while (graines > 0) {
        courante = courante->caseSuiv;
        if (courante == depart) continue;

        if (couleur == 1) {
            courante->R++;
            graines--;
        } else if (couleur == 2) {
            if (!case_du_joueur(courante->caseN, joueur)) {
                courante->B++;
                graines--;
            }
        } else if (couleur == 3) {
            if (mode_transp == 1) { // joue comme rouge
                courante->T++;
                graines--;
            } else if (mode_transp == 2 && !case_du_joueur(courante->caseN, joueur)) { // joue comme bleue
                courante->T++;
                graines--;
            }
        }

        derniere = courante;
    }

    return derniere;
}

// === MAIN ===
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
        int num_case = 0;
        int couleur = 0;
        int mode_transp = 0;

        printf("Entre ton coup (ex : 16B, 8R, 3TB, 5TR) : ");
        scanf("%s", entree);

        // Analyse de la chaîne
        int i = 0;
        while (isdigit(entree[i])) i++;
        num_case = atoi(entree);

        if (entree[i] == 'R' || entree[i] == 'r') couleur = 1;
        else if (entree[i] == 'B' || entree[i] == 'b') couleur = 2;
        else if (entree[i] == 'T' || entree[i] == 't') {
            couleur = 3;
            i++;
            if (entree[i] == 'R' || entree[i] == 'r') mode_transp = 1;
            else if (entree[i] == 'B' || entree[i] == 'b') mode_transp = 2;
        }

        Plateau* c = trouver_case(plateau, num_case);
        if (!c || !case_du_joueur(num_case, tour + 1)) {
            printf("Case invalide pour ce joueur.\n");
            continue;
        }

        Plateau* derniere = distribuer(plateau, c, couleur, tour + 1, nb_cases, mode_transp);
        int captures = capturer(plateau, derniere, nb_cases, tour + 1);
        scores[tour] += captures;

        printf("%s a capturé %d graines.\n", joueurs[tour], captures);
        afficher_plateau(plateau, nb_cases);
        printf("Scores -> J1: %d | J2: %d\n", scores[0], scores[1]);
        printf("----------------------------------------\n");

        tour = 1 - tour;
    }

    return 0;
}
