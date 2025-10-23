#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"

#define TEMPS_MAX 2.0  // Temps max par coup (en secondes)

// --- Déclarations ---
int total_graines(Plateau* plateau, int nb_cases);
int total_graines_joueur(Plateau* plateau, int nb_cases, int joueur);

// --- Structures de jeu ---
typedef struct {
    Plateau* plateau;
    int scores[2];
    int tour; // 0 = J1, 1 = J2
    int nb_cases;
} Partie;

// --- Fonctions ---
void initialiser_partie(Partie* partie);
int jouer_tour(Partie* partie);
void afficher_resultats(Partie* partie);
int verifier_conditions_fin(Partie* partie);
int choisir_meilleur_coup(Partie* partie, int joueur, int* best_case, int* best_color, int* best_mode);
void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode);

// ============================
//        PROGRAMME PRINCIPAL
// ============================

int main() {
    Partie partie;
    initialiser_partie(&partie);

    printf("=== Début du match IA vs IA ===\n");
    afficher_plateau(partie.plateau, partie.nb_cases);

    while (1) {
        if (!jouer_tour(&partie)) break; // Retourne 0 si fin de partie
        if (verifier_conditions_fin(&partie)) break;
        printf("----------------------------------------\n");
        partie.tour = 1 - partie.tour;
    }

    afficher_resultats(&partie);
    liberer_plateau(partie.plateau, partie.nb_cases);
    return 0;
}


void initialiser_partie(Partie* partie) {
    partie->nb_cases = 16;
    partie->plateau = creer_plateau(partie->nb_cases);
    partie->scores[0] = partie->scores[1] = 0;
    partie->tour = 0;
    initialiser_killer_moves();
}

int jouer_tour(Partie* partie) {
    int joueur = partie->tour + 1;
    char* noms[2] = {"J1 (classique)", "J2 (famine)"};
    printf("\n=== Tour de %s ===\n", noms[partie->tour]);

    int best_case = -1, best_color = -1, best_mode = 0;

    // --- Sélection du meilleur coup ---
    int ok = choisir_meilleur_coup(partie, joueur, &best_case, &best_color, &best_mode);
    if (!ok) {
        printf("⚠️  Aucun coup possible pour %s.\n", noms[partie->tour]);
        return 0;
    }

    // --- Exécution du coup ---
    executer_coup(partie, joueur, best_case, best_color, best_mode);
    afficher_plateau(partie->plateau, partie->nb_cases);

    printf("Scores -> J1: %d | J2: %d\n", partie->scores[0], partie->scores[1]);
    int evalJ1 = evaluer_plateau(partie->plateau, 1);
    int evalJ2 = evaluer_plateau_famine(partie->plateau, 2);
    printf("Évaluation -> J1: %d | J2: %d\n", evalJ1, evalJ2);
    return 1;
}

int choisir_meilleur_coup(Partie* partie, int joueur, int* best_case, int* best_color, int* best_mode) {
    int nb_cases = partie->nb_cases;
    Plateau* plateau = partie->plateau;
    struct timeval debut, courant;
    gettimeofday(&debut, NULL);

    int meilleur_score = -9999;
    int profondeur_finale = 1;
    double duree_totale = 0.0;

    for (int d = 1; d <= 15; d++) {
        int meilleur_score_temp = -9999;
        int best_case_temp = -1, best_color_temp = -1, best_mode_temp = 0;
        int coups_eval = 0;

        Plateau* p = plateau;
        for (int i = 0; i < nb_cases; i++) {
            if (!case_du_joueur(p->caseN, joueur)) { p = p->caseSuiv; continue; }
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

                    int s = minimax(copie, joueur, d, -100000, 100000, 1);
                    s += captures * 5;
                    coups_eval++;

                    if (s > meilleur_score_temp) {
                        meilleur_score_temp = s;
                        best_case_temp = p->caseN;
                        best_color_temp = couleur;
                        best_mode_temp = (couleur == 3 ? modes[m] : 0);
                    }

                    liberer_plateau(copie, nb_cases);
                    gettimeofday(&courant, NULL);
                    duree_totale = (courant.tv_sec - debut.tv_sec) +
                                   (courant.tv_usec - debut.tv_usec) / 1000000.0;
                    if (duree_totale >= TEMPS_MAX && coups_eval > 0)
                        goto FIN_TEMPS;
                }
            }
            p = p->caseSuiv;
        }

        if (best_case_temp != -1) {
            profondeur_finale = d;
            meilleur_score = meilleur_score_temp;
            *best_case = best_case_temp;
            *best_color = best_color_temp;
            *best_mode = best_mode_temp;
        }

        gettimeofday(&courant, NULL);
        duree_totale = (courant.tv_sec - debut.tv_sec) +
                       (courant.tv_usec - debut.tv_usec) / 1000000.0;

        printf("🔹 Profondeur %d terminée en %.3fs | Meilleur coup : ", d, duree_totale);

        if (*best_color == 1)
            printf("%dR", *best_case);
        else if (*best_color == 2)
            printf("%dB", *best_case);
        else if (*best_color == 3 && *best_mode == 1)
            printf("%dTR", *best_case);
        else if (*best_color == 3 && *best_mode == 2)
            printf("%dTB", *best_case);

        printf("\n");
        fflush(stdout);


        if (duree_totale >= TEMPS_MAX) break;
    }

FIN_TEMPS:
    printf("✅ Profondeur finale retenue : %d (%.3fs)\n", profondeur_finale, duree_totale);
    return (*best_case != -1);
}


void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode) {
    char* noms[2] = {"J1 (classique)", "J2 (famine)"};
    Plateau* c = trouver_case(partie->plateau, best_case);
    Plateau* derniere = distribuer(c, best_color, joueur, best_mode);
    int captures = capturer(partie->plateau, derniere, partie->nb_cases);
    partie->scores[joueur - 1] += captures;

    printf("👉 %s joue ", noms[joueur - 1]);
    if (best_color == 1) printf("%dR", best_case);
    else if (best_color == 2) printf("%dB", best_case);
    else if (best_color == 3 && best_mode == 1) printf("%dTR", best_case);
    else if (best_color == 3 && best_mode == 2) printf("%dTB", best_case);
    printf(" et capture %d graines.\n", captures);
}


int verifier_conditions_fin(Partie* partie) {
    int totalJ1 = total_graines_joueur(partie->plateau, partie->nb_cases, 1);
    int totalJ2 = total_graines_joueur(partie->plateau, partie->nb_cases, 2);
    int total = totalJ1 + totalJ2;

    if (totalJ1 == 0 && totalJ2 > 0) {
        printf("💀 Famine ! J2 récupère toutes les graines restantes (%d).\n", totalJ2);
        partie->scores[1] += totalJ2;
        return 1;
    }
    if (totalJ2 == 0 && totalJ1 > 0) {
        printf("💀 Famine ! J1 récupère toutes les graines restantes (%d).\n", totalJ1);
        partie->scores[0] += totalJ1;
        return 1;
    }
    if (total < 10) {
        printf("⚖️  Moins de 10 graines restantes — fin de la partie.\n");
        return 1;
    }
    return 0;
}


void afficher_resultats(Partie* partie) {
    printf("\n=== Fin de la partie ===\n");
    printf("Score final -> J1: %d | J2: %d\n", partie->scores[0], partie->scores[1]);
    if (partie->scores[0] > partie->scores[1])
        printf("🏆 Victoire de J1 (classique)\n");
    else if (partie->scores[1] > partie->scores[0])
        printf("🏆 Victoire de J2 (famine)\n");
    else
        printf("🤝 Match nul\n");
}
