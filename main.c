#include <stdio.h>
#include <stdlib.h>
#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"

// Fonctions utilitaires (si non déjà présentes)
int total_graines(Plateau* plateau, int nb_cases);
int total_graines_joueur(Plateau* plateau, int nb_cases, int joueur);

// === Simulation IA vs IA ===
// J1 utilise evaluer_plateau
// J2 utilise evaluer_plateau_famine

int main() {
    int nb_cases = 16;
    Plateau* plateau = creer_plateau(nb_cases);
    int scores[2] = {0, 0};
    int tour = 0; // 0 = J1, 1 = J2
    char* joueurs[2] = {"J1 (classique)", "J2 (famine)"};

    printf("=== Début du match IA vs IA ===\n");
    afficher_plateau(plateau, nb_cases);

    while (1) {
        int joueur = tour + 1;

        // Calcul des totaux avant le tour
        int totalJ1 = total_graines_joueur(plateau, nb_cases, 1);
        int totalJ2 = total_graines_joueur(plateau, nb_cases, 2);
        int total = totalJ1 + totalJ2;

        printf("\n=== Tour de %s ===\n", joueurs[tour]);

        // Sélection du meilleur coup
        int nb_cases_plateau = nb_cases;
        int meilleur_score = -9999;
        int best_case = -1;
        int best_color = -1;
        int best_mode = 0;

        Plateau* p = plateau;
        for (int i = 0; i < nb_cases; i++) {
            if (case_du_joueur(p->caseN, joueur)) {
                int couleurs[3] = {1, 2, 3};
                for (int c = 0; c < 3; c++) {
                    int couleur = couleurs[c];
                    if ((couleur == 1 && p->R == 0) ||
                        (couleur == 2 && p->B == 0) ||
                        (couleur == 3 && p->T == 0))
                        continue;

                    int modes[2] = {1, 2};
                    int nb_modes = (couleur == 3) ? 2 : 1;

                    for (int m = 0; m < nb_modes; m++) {
                        Plateau* copie = copier_plateau(plateau, nb_cases);
                        Plateau* case_copie = trouver_case(copie, p->caseN);
                        Plateau* derniere = distribuer(case_copie, couleur, joueur,
                                                       (couleur == 3 ? modes[m] : 0));

                        int captures = capturer(copie, derniere, nb_cases);

                        int score;
                        if (joueur == 1)
                            score = minimax(copie, joueur, 5, -100000, 100000, 1);
                        else
                            score = minimax(copie, joueur, 5, -100000, 100000, 1);

                        if (score > meilleur_score) {
                            meilleur_score = score;
                            best_case = p->caseN;
                            best_color = couleur;
                            best_mode = (couleur == 3 ? modes[m] : 0);
                        }

                        liberer_plateau(copie, nb_cases);
                    }
                }
            }
            p = p->caseSuiv;
        }

        if (best_case == -1) {
            printf("⚠️  Aucun coup possible pour %s.\n", joueurs[tour]);
            break;
        }

        // Exécution du meilleur coup
        Plateau* c = trouver_case(plateau, best_case);
        Plateau* derniere = distribuer(c, best_color, joueur, best_mode);
        int captures = capturer(plateau, derniere, nb_cases);
        scores[tour] += captures;

        printf("👉 %s joue ", joueurs[tour]);
        if (best_color == 1)
            printf("%dR", best_case);
        else if (best_color == 2)
            printf("%dB", best_case);
        else if (best_color == 3 && best_mode == 1)
            printf("%dTR", best_case);
        else if (best_color == 3 && best_mode == 2)
            printf("%dTB", best_case);

        printf(" et capture %d graines (score local %d)\n", captures, meilleur_score);

        afficher_plateau(plateau, nb_cases);
        printf("Scores -> J1: %d | J2: %d\n", scores[0], scores[1]);

        int evalJ1 = evaluer_plateau(plateau, 1);
        int evalJ2 = evaluer_plateau_famine(plateau, 2);
        printf("Évaluation du plateau -> J1: %d | J2: %d\n", evalJ1, evalJ2);

        // Vérifications de fin de partie
        totalJ1 = total_graines_joueur(plateau, nb_cases, 1);
        totalJ2 = total_graines_joueur(plateau, nb_cases, 2);
        total = totalJ1 + totalJ2;

        if (totalJ1 == 0 && totalJ2 > 0) {
            printf("💀 Famine ! J2 récupère toutes les graines restantes (%d).\n", totalJ2);
            scores[1] += totalJ2;
            break;
        }
        if (totalJ2 == 0 && totalJ1 > 0) {
            printf("💀 Famine ! J1 récupère toutes les graines restantes (%d).\n", totalJ1);
            scores[0] += totalJ1;
            break;
        }
        if (total < 10) {
            printf("⚖️  Moins de 10 graines restantes — fin de la partie.\n");
            break;
        }

        printf("----------------------------------------\n");
        tour = 1 - tour;
    }

    printf("\n=== Fin de la partie ===\n");
    printf("Score final -> J1: %d | J2: %d\n", scores[0], scores[1]);
    if (scores[0] > scores[1]) printf("🏆 Victoire de J1 (classique)\n");
    else if (scores[1] > scores[0]) printf("🏆 Victoire de J2 (famine)\n");
    else printf("🤝 Match nul\n");

    liberer_plateau(plateau, nb_cases);
    return 0;
}
