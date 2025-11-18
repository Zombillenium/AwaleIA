#include "ia.h"
#include <sys/time.h>   // pour struct timeval et gettimeofday
#include "plateau.h"
#include "jeu.h"
#include "evaluation.h"
#include "tabletranspo.h"
#include "tuning.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define MAX_PROFONDEUR 32
#define MAX_KILLERS 2

// killer_moves déclaré UNE SEULE FOIS ici, dans ia.c
static Coup killer_moves[MAX_PROFONDEUR][MAX_KILLERS];

#ifdef _OPENMP
#pragma omp threadprivate(killer_moves)
#endif

#define TEMPS_MAX 2.00  // Temps max par coup (en secondes)


// =============================
// SECTION 1 : OUTILS GÉNÉRAUX
// =============================

// Copie complète d’un plateau
Plateau* copier_plateau(Plateau* original) {
    Plateau* copie = creer_plateau();
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

// Trie les cases selon le nombre de graines (ordre décroissant)
void trier_cases_par_valeur(Plateau* plateau, int* ordre, int* valeurs) {
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
                         int profondeur,
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

        jouer_coup(plateau, km);
        int score = minimax(plateau, joueur, profondeur - 1, *alpha, *beta, !maximisant);
        score += (maximisant ? km->captures * 5 : -km->captures * 5);
        annuler_coup(plateau, km);

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


// =============================
//   CHOIX DU MEILLEUR COUP
// =============================

int choisir_meilleur_coup(Partie* partie, int joueur,
                          int* best_case, int* best_color, int* best_mode) {

    Plateau* plateau = partie->plateau;

    struct timeval debut;
    gettimeofday(&debut, NULL);

    #ifdef _OPENMP
    omp_set_nested(0);
    omp_set_dynamic(0);
    #endif

    #ifdef _OPENMP
    #pragma omp parallel
    {
        initialiser_killer_moves();
    }
    #else
    initialiser_killer_moves();
    #endif

    double duree_derniere = 0.0;
    int profondeur_finale = 1;
    Move best_previous = {-1, -1, 0};
    int last_score = 0;
    int have_last = 0;

    for (int d = 1; d <= 25; d++) {

        Move moves[nb_cases * 3 * 2];
        int nmoves = 0;
        Plateau* p = plateau;

        for (int i = 0; i < nb_cases; i++) {
            if (!case_du_joueur(p->caseN, joueur)) {
                p = p->caseSuiv;
                continue; // ne pas jouer les cases de l’adversaire
            }

            // Filtrage des coups impossibles (cases vides)
            if (p->R > 0) {
                moves[nmoves++] = (Move){p->caseN, 1, 0}; // Rouge seul
            }
            if (p->B > 0) {
                moves[nmoves++] = (Move){p->caseN, 2, 0}; // Bleu seul
            }
            if (p->T > 0) {
                // TR : autorisé si T > 0 ou R > 0
                if (p->T + p->R > 0)
                    moves[nmoves++] = (Move){p->caseN, 3, 1};
                // TB : autorisé si T > 0 ou B > 0
                if (p->T + p->B > 0)
                    moves[nmoves++] = (Move){p->caseN, 3, 2};
            }

            p = p->caseSuiv;
        }

        if (nmoves == 0) return 0;

        // priorité au coup précédent
        if (best_previous.caseN != -1) {
            for (int i = 0; i < nmoves; i++) {
                if (moves[i].caseN == best_previous.caseN &&
                    moves[i].couleur == best_previous.couleur &&
                    moves[i].mode == best_previous.mode) {
                    Move tmp = moves[0]; moves[0] = moves[i]; moves[i] = tmp;
                    break;
                }
            }
        }

        // aspiration
        int ASP = 75;
        int alpha_root = have_last ? (last_score - ASP) : -100000;
        int beta_root  = have_last ? (last_score + ASP) :  100000;

        int best_case_temp = -1, best_color_temp = -1, best_mode_temp = 0;
        int meilleur_score_local = -999999;

        for (;;) {
            best_case_temp = -1; best_color_temp = -1; best_mode_temp = 0;
            meilleur_score_local = -999999;

            struct timeval t1, t2;
            gettimeofday(&t1, NULL);

            #ifdef _OPENMP
            #pragma omp parallel for schedule(dynamic,1) reduction(max:meilleur_score_local)
            #endif
            for (int k = 0; k < nmoves; k++) {
                Move mv = moves[k];
                Plateau* copie = copier_plateau(plateau);
                Plateau* case_copie = trouver_case(copie, mv.caseN);
                Plateau* derniere = distribuer(case_copie, mv.couleur, joueur,
                                               (mv.couleur == 3 ? mv.mode : 0));
                int captures = capturer(copie, derniere);

                int score = minimax(copie, joueur, d, alpha_root, beta_root, 1) + captures * 5;

                #ifdef _OPENMP
                #pragma omp critical
                #endif
                {
                    if (score > meilleur_score_local) {
                        meilleur_score_local = score;
                        best_case_temp  = mv.caseN;
                        best_color_temp = mv.couleur;
                        best_mode_temp  = (mv.couleur == 3 ? mv.mode : 0);
                    }
                }
                liberer_plateau(copie);
            }

            gettimeofday(&t2, NULL);
            double duree = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
            duree_derniere = duree;

            printf("🔹 Profondeur %d terminée en %.3fs | Meilleur coup : ",
                   d, duree);
            if (best_color_temp == 1) printf("%dR", best_case_temp);
            else if (best_color_temp == 2) printf("%dB", best_case_temp);
            else if (best_color_temp == 3 && best_mode_temp == 1) printf("%dTR", best_case_temp);
            else if (best_color_temp == 3 && best_mode_temp == 2) printf("%dTB", best_case_temp);
            printf(" (score %d)", meilleur_score_local);

            if (have_last && meilleur_score_local <= alpha_root) {
                ASP *= 2;
                alpha_root = last_score - ASP;
                beta_root  = last_score + ASP;
                printf(" ⤵️ fail-low → [%d,%d]\n", alpha_root, beta_root);
                fflush(stdout);
                continue;
            } else if (have_last && meilleur_score_local >= beta_root) {
                ASP *= 2;
                alpha_root = last_score - ASP;
                beta_root  = last_score + ASP;
                printf(" ⤴️ fail-high → [%d,%d]\n", alpha_root, beta_root);
                fflush(stdout);
                continue;
            } else {
                printf("\n");
                break;
            }
        }

        *best_case = best_case_temp;
        *best_color = best_color_temp;
        *best_mode = best_mode_temp;
        profondeur_finale = d;
        best_previous = (Move){*best_case, *best_color, *best_mode};

        last_score = (meilleur_score_local == -999999) ? 0 : meilleur_score_local;
        have_last = 1;

        if (duree_derniere * 8.0 >= TEMPS_MAX) {
            printf("⏱️ Prochaine profondeur estimée à %.3fs → arrêt anticipé.\n",
                   duree_derniere * 8.0);
            break;
        }
    }

    printf("✅ Profondeur finale retenue : %d\n", profondeur_finale);
    return (*best_case != -1);
}



// =============================
//        MINIMAX
// =============================

// Version avec paramètres tunés
int minimax_tune(Plateau* plateau, int joueur, int profondeur,
                 int alpha, int beta, int maximisant, ParametresEvaluation* params) {

    unsigned long long key = hash_plateau(plateau);
    int val_cache;
    if (chercher_transpo(joueur, key, profondeur, &val_cache)) return val_cache;

    // === TERMINAISONS EXPLICITES (win-fast, lose-late) ===
    int terminal = evaluer_fin_de_partie(plateau, joueur);
    if (terminal == SCORE_VICTOIRE) {
        int bonus = profondeur * 50;
        ajouter_transpo(joueur, key, profondeur, SCORE_VICTOIRE - bonus);
        return SCORE_VICTOIRE - bonus;
    }
    if (terminal == SCORE_DEFAITE) {
        int malus = profondeur * 50;
        ajouter_transpo(joueur, key, profondeur, SCORE_DEFAITE + malus);
        return SCORE_DEFAITE + malus;
    }

    if (profondeur == 0) {
        // Utilisation des fonctions d'évaluation avec paramètres tunés
        int eval;
        if (params) {
            eval = (joueur == 1)
                ? evaluer_plateau_avance_tune(plateau, joueur, params)
                : evaluer_plateau_famine_avance_tune(plateau, joueur, params);
        } else {
            eval = (joueur == 1)
                ? evaluer_plateau_avance(plateau, joueur)
                : evaluer_plateau_famine_avance(plateau, joueur);
        }
        ajouter_transpo(joueur, key, profondeur, eval);
        return eval;
    }

    int meilleur_score = maximisant ? -99999 : 99999;
    int coup_trouve = 0;
    int joueur_actuel = maximisant ? joueur : 3 - joueur;

    // On essaie d'abord les killer moves
    tester_killer_moves(plateau, joueur, joueur_actuel, profondeur,
                        &meilleur_score, &alpha, &beta, maximisant);

    // On trie les cases par valeur
    int ordre[nb_cases], valeurs[nb_cases];
    trier_cases_par_valeur(plateau, ordre, valeurs);

    // Exploration des coups
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

                jouer_coup(plateau, &coup);
                coup_trouve = 1;
                int score = minimax_tune(plateau, joueur, profondeur - 1,
                                        alpha, beta, !maximisant, params);
                score += (maximisant ? coup.captures * 5 : -coup.captures * 5);
                annuler_coup(plateau, &coup);

                if (maximisant) {
                    if (score > meilleur_score) meilleur_score = score;
                    if (meilleur_score > alpha) alpha = meilleur_score;
                } else {
                    if (score < meilleur_score) meilleur_score = score;
                    if (meilleur_score < beta) beta = meilleur_score;
                }

                if (beta <= alpha) {
                    enregistrer_killer_move(profondeur, coup);
                    goto FIN_COUPE_TUNE;
                }
            }
        }
    }

FIN_COUPE_TUNE:
    if (!coup_trouve) {
        int terminal = evaluer_fin_de_partie(plateau, joueur);
        if (terminal == SCORE_VICTOIRE || terminal == SCORE_DEFAITE) {
            if (terminal == SCORE_VICTOIRE) {
                meilleur_score = SCORE_VICTOIRE - profondeur * 50;
            } else {
                meilleur_score = SCORE_DEFAITE + profondeur * 50;
            }
        } else {
            int eval;
            if (params) {
                eval = (joueur == 1)
                    ? evaluer_plateau_avance_tune(plateau, joueur, params)
                    : evaluer_plateau_famine_avance_tune(plateau, joueur, params);
            } else {
                eval = (joueur == 1)
                    ? evaluer_plateau_avance(plateau, joueur)
                    : evaluer_plateau_famine_avance(plateau, joueur);
            }
            meilleur_score = eval;
        }
    }
    ajouter_transpo(joueur, key, profondeur, meilleur_score);
    return meilleur_score;
}

// Version standard (sans paramètres tunés)
int minimax(Plateau* plateau, int joueur, int profondeur,
            int alpha, int beta, int maximisant) {
    return minimax_tune(plateau, joueur, profondeur, alpha, beta, maximisant, NULL);
}
