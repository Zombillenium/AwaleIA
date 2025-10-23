#include "ia.h"
#include "plateau.h"
#include "jeu.h"
#include "evaluation.h"
#include "tabletranspo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#define MAX_PROFONDEUR 32
#define MAX_KILLERS 2

// ✅ killer_moves déclaré UNE SEULE FOIS ici, dans ia.c
static Coup killer_moves[MAX_PROFONDEUR][MAX_KILLERS];

#ifdef _OPENMP
#pragma omp threadprivate(killer_moves)
#endif

// =============================
// SECTION 1 : OUTILS GÉNÉRAUX
// =============================

// Copie complète d’un plateau
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

// Réinitialisation des killer moves
void initialiser_killer_moves() {
    memset(killer_moves, 0, sizeof(killer_moves));
}

// === Trie les cases selon le nombre de graines (ordre décroissant)
void trier_cases_par_valeur(Plateau* plateau, int nb_cases, int* ordre, int* valeurs) {
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
}




// Enregistre un killer move lors d'une coupe beta
void enregistrer_killer_move(int profondeur, Coup coup) {
    Coup *k0 = &killer_moves[profondeur][0];
    Coup *k1 = &killer_moves[profondeur][1];

    // Vérifie si le coup existe déjà
    if (k0->case_depart == coup.case_depart &&
        k0->couleur == coup.couleur &&
        k0->mode_transp == coup.mode_transp) return;

    // Décale
    *k1 = *k0;
    *k0 = coup;
}




// Explore d’abord les killer moves à cette profondeur
void tester_killer_moves(Plateau* plateau, int joueur, int joueur_actuel,
                         int profondeur, int nb_cases,
                         int* meilleur_score, int* alpha, int* beta, int maximisant) {

    for (int k = 0; k < MAX_KILLERS; k++) {
        Coup* km = &killer_moves[profondeur][k];
        if (km->case_depart == 0) continue;

        Plateau* c = trouver_case(plateau, km->case_depart);
        if (!case_du_joueur(c->caseN, joueur_actuel)) continue;

        int couleur = km->couleur;

        if ((couleur == 1 && c->R == 0) ||
            (couleur == 2 && c->B == 0) ||
            (couleur == 3 && c->T == 0))
            continue;

        jouer_coup(plateau, km, nb_cases);
        int score = minimax(plateau, joueur, profondeur - 1, *alpha, *beta, !maximisant);
        score += (maximisant ? km->captures * 5 : -km->captures * 5);
        annuler_coup(plateau, km, nb_cases);

        if (maximisant) {
            if (score > *meilleur_score) *meilleur_score = score;
            if (*meilleur_score > *alpha) *alpha = *meilleur_score;
        } else {
            if (score < *meilleur_score) *meilleur_score = score;
            if (*meilleur_score < *beta) *beta = *meilleur_score;
        }

        if (*beta <= *alpha) return; // coupe immédiate
    }
}







int minimax(Plateau* plateau, int joueur, int profondeur, int alpha, int beta, int maximisant) {

    int nb_cases = 16;

    unsigned long long key = hash_plateau(plateau, nb_cases);
    int val_cache;
    if (chercher_transpo(joueur, key, profondeur, &val_cache)) return val_cache;

    if (profondeur == 0) {
        int eval = (joueur  == 1)
            ? evaluer_plateau(plateau, joueur )
            : evaluer_plateau_famine(plateau, joueur );
        ajouter_transpo(joueur, key, profondeur, eval);
        return eval;
    }

    int meilleur_score = maximisant ? -99999 : 99999;
    int joueur_actuel = maximisant ? joueur : 3 - joueur;

    // 🔹 On essaie d’abord les killer moves
    tester_killer_moves(plateau, joueur, joueur_actuel, profondeur,
                        nb_cases, &meilleur_score, &alpha, &beta, maximisant);

    // 🔹 On trie les cases par valeur
    int ordre[16], valeurs[16];
    trier_cases_par_valeur(plateau, nb_cases, ordre, valeurs);

    // 🔹 Exploration des coups
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
                Coup coup = {
                    .case_depart = c->caseN,
                    .couleur = couleur,
                    .mode_transp = (couleur == 3 ? modes[m] : 0),
                    .joueur = joueur_actuel
                };

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

                if (beta <= alpha) {
                    enregistrer_killer_move(profondeur, coup);
                    goto FIN_COUPE;
                }
            }
        }
    }

FIN_COUPE:
    ajouter_transpo(joueur, key, profondeur, meilleur_score);
    return meilleur_score;
}




