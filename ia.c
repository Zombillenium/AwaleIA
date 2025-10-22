#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ia.h"
#include "plateau.h"
#include "jeu.h"
#include "evaluation.h"
#include "tabletranspo.h"

// === Copie complète d’un plateau circulaire ===
Plateau* copier_plateau(Plateau* original, int nb_cases) {
    Plateau* copie = creer_plateau(nb_cases);
    Plateau* p1 = original;
    Plateau* p2 = copie;

    for (int i = 0; i < nb_cases; i++) {
        p2->R = p1->R;
        p2->B = p1->B;
        p2->T = p1->T;
        p1 = p1->caseSuiv;
        p2 = p2->caseSuiv;
    }

    return copie;
}

// === Recherche du meilleur coup immédiat ===
void meilleur_coup(Plateau* plateau, int joueur) {
    int nb_cases = 16;
    int meilleur_score = -9999;
    int best_case = -1;
    int best_color = -1;
    int best_mode = 0;

    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        if (case_du_joueur(p->caseN, joueur)) {
            int couleurs[3] = {1, 2, 3}; // Rouge, Bleu, Transparent
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
                    int score = evaluer_plateau(copie, joueur) + captures * 5; // bonus captures

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

    printf("\n👉 Meilleur coup suggéré pour %s : ",
           (joueur == 1 ? "J1" : "J2"));

    if (best_case == -1) {
        printf("aucun coup possible !\n");
        return;
    }

    if (best_color == 1)
        printf("%dR (score %d)\n", best_case, meilleur_score);
    else if (best_color == 2)
        printf("%dB (score %d)\n", best_case, meilleur_score);
    else if (best_color == 3 && best_mode == 1)
        printf("%dTR (score %d)\n", best_case, meilleur_score);
    else if (best_color == 3 && best_mode == 2)
        printf("%dTB (score %d)\n", best_case, meilleur_score);
}

int minimax(Plateau* plateau, int joueur, int profondeur, int alpha, int beta, int maximisant) {
    int nb_cases = 16;

    // Vérifie la table de transposition
    unsigned long long key = hash_plateau(plateau, nb_cases);
    int val_cache;
    if (chercher_transpo(key, profondeur, &val_cache)) return val_cache;

    if (profondeur == 0) {
        int eval = (joueur == 1)
            ? evaluer_plateau(plateau, joueur)
            : evaluer_plateau_famine(plateau, joueur);
        ajouter_transpo(key, profondeur, eval);
        return eval;
    }

    int meilleur_score = maximisant ? -99999 : 99999;
    int joueur_actuel = maximisant ? joueur : 3 - joueur;
    Plateau* p = plateau;

    // --- Tri rapide : on explore d’abord les cases les plus remplies ---
    int ordre[16];
    int valeurs[16];
    for (int i = 0; i < nb_cases; i++) {
        ordre[i] = i + 1;
        Plateau* tmp = trouver_case(plateau, i + 1);
        valeurs[i] = tmp->R + tmp->B + tmp->T;
    }
    for (int i = 0; i < nb_cases - 1; i++) {
        for (int j = i + 1; j < nb_cases; j++) {
            if (valeurs[j] > valeurs[i]) {
                int t = valeurs[i]; valeurs[i] = valeurs[j]; valeurs[j] = t;
                t = ordre[i]; ordre[i] = ordre[j]; ordre[j] = t;
            }
        }
    }

    for (int idx = 0; idx < nb_cases; idx++) {
        Plateau* c = trouver_case(plateau, ordre[idx]);
        if (!case_du_joueur(c->caseN, joueur_actuel)) continue;

        int couleurs[3] = {1, 2, 3};
        for (int co = 0; co < 3; co++) {
            int couleur = couleurs[co];
            if ((couleur == 1 && c->R == 0) ||
                (couleur == 2 && c->B == 0) ||
                (couleur == 3 && c->T == 0))
                continue;

            int modes[2] = {1, 2};
            int nb_modes = (couleur == 3) ? 2 : 1;

            for (int m = 0; m < nb_modes; m++) {
                Coup coup = { .case_depart = c->caseN, .couleur = couleur, .mode_transp = (couleur == 3 ? modes[m] : 0), .joueur = joueur_actuel };
                jouer_coup(plateau, &coup, nb_cases);

                int score = minimax(plateau, joueur, profondeur - 1, alpha, beta, !maximisant);
                score += (maximisant ? coup.captures * 5 : -coup.captures * 5);

                annuler_coup(plateau, &coup, nb_cases);

                if (maximisant) {
                    if (score > meilleur_score) meilleur_score = score;
                    if (meilleur_score > alpha) alpha = meilleur_score;
                } else {
                    if (score < meilleur_score) meilleur_score = score;
                    if (meilleur_score < beta) beta = meilleur_score;
                }

                if (beta <= alpha) goto FIN_COUPE;
            }
        }
    }

FIN_COUPE:
    ajouter_transpo(key, profondeur, meilleur_score);
    return meilleur_score;
}
