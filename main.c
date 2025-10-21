#include <stdio.h>
#include <stdlib.h>

// === Structures ===
typedef struct Plateau {
    int caseN;          // Numéro de la case
    int R;              // Graines rouges
    int B;              // Graines bleues
    int T;              // Graines transparentes
    struct Plateau* caseSuiv;  // Pointeur vers la case suivante (cercle)
} Plateau;

// === Fonctions utilitaires ===

// Création du plateau circulaire
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

        if (precedente != NULL)
            precedente->caseSuiv = nouvelle;
        else
            premiere = nouvelle;

        precedente = nouvelle;
    }

    precedente->caseSuiv = premiere; // boucle
    return premiere;
}

// Affichage du plateau
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

// Trouver une case donnée par numéro
Plateau* trouver_case(Plateau* plateau, int num) {
    Plateau* p = plateau;
    do {
        if (p->caseN == num) return p;
        p = p->caseSuiv;
    } while (p != plateau);
    return NULL;
}

// Retourne la case précédente (dans le cercle)
Plateau* case_precedente(Plateau* plateau, Plateau* cible, int nb_cases) {
    Plateau* p = plateau;
    while (p->caseSuiv != cible) {
        p = p->caseSuiv;
    }
    return p;
}

// === Règles de jeu ===

// Vérifie si une case appartient au joueur (1 -> impair, 2 -> pair)
int case_du_joueur(int caseN, int joueur) {
    if (joueur == 1) return (caseN % 2 == 1);
    else return (caseN % 2 == 0);
}

// Fonction de capture
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

// Distribution
Plateau* distribuer(Plateau* plateau, Plateau* depart, int couleur, int joueur, int nb_cases) {
    Plateau* courante = depart;
    int graines = 0;

    // Déterminer le nombre de graines à distribuer
    if (couleur == 1) { // rouge
        graines = depart->R;
        depart->R = 0;
    } else if (couleur == 2) { // bleue
        graines = depart->B;
        depart->B = 0;
    } else if (couleur == 3) { // transparente
        graines = depart->T;
        depart->T = 0;
    }

    Plateau* derniere = NULL;
    while (graines > 0) {
        courante = courante->caseSuiv;

        // Ne pas distribuer dans la case de départ
        if (courante == depart) continue;

        // Rouge : distribue dans toutes les cases
        if (couleur == 1) {
            courante->R++;
            graines--;
        }
        // Bleue : distribue seulement dans les cases adverses
        else if (couleur == 2) {
            if (!case_du_joueur(courante->caseN, joueur)) {
                courante->B++;
                graines--;
            }
        }
        // Transparente : choix du comportement
        else if (couleur == 3) {
            // On demande au joueur le mode
            int choix;
            printf("Choisis le mode de distribution des transparentes (1=Rouge / 2=Bleue): ");
            scanf("%d", &choix);
            // D’abord distribuer les transparentes selon le choix
            if (choix == 1) {
                courante->T++;
                graines--;
            } else {
                if (!case_du_joueur(courante->caseN, joueur)) {
                    courante->T++;
                    graines--;
                }
            }
        }

        derniere = courante;
    }

    return derniere;
}

// === Fonction principale ===
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
        int num_case, couleur;

        // Choisir une case
        do {
            printf("Choisis une case (1-16) appartenant à %s : ", joueurs[tour]);
            scanf("%d", &num_case);
        } while (!case_du_joueur(num_case, tour + 1));

        Plateau* c = trouver_case(plateau, num_case);

        // Choisir la couleur
        printf("Choisis une couleur à jouer : 1=Rouge  2=Bleue  3=Transparente : ");
        scanf("%d", &couleur);

        Plateau* derniere = distribuer(plateau, c, couleur, tour + 1, nb_cases);
        int captures = capturer(plateau, derniere, nb_cases, tour + 1);
        scores[tour] += captures;

        printf("%s a capturé %d graines.\n", joueurs[tour], captures);
        afficher_plateau(plateau, nb_cases);

        printf("Scores -> J1: %d | J2: %d\n", scores[0], scores[1]);
        printf("----------------------------------------\n");

        tour = 1 - tour; // Changement de joueur
    }

    return 0;
}
