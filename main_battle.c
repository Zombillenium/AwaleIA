#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"
#include "tabletranspo.h"
#include <omp.h>

#define TEMPS_MAX 2.0  // Temps max par coup (en secondes)

// --- Structures de jeu ---
typedef struct {
    Plateau* plateau;
    int scores[2];
    int tour; // 0 = humain, 1 = IA
    int nb_cases;
} Partie;

typedef struct {
    int caseN;
    int couleur;  // 1=R, 2=B, 3=T
    int mode;     // pour T: 1=TR, 2=TB; sinon 0
} Move;

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
    printf("💻 OpenMP détecte %d threads disponibles.\n", omp_get_max_threads());
    Partie partie;
    initialiser_partie(&partie);
    initialiser_transpo_lock();

    printf("=== Début du match Humain (J1) vs IA famine (J2) ===\n");
    afficher_plateau(partie.plateau, partie.nb_cases);

    while (1) {
        if (!jouer_tour(&partie)) break;
        if (verifier_conditions_fin(&partie)) break;
        printf("----------------------------------------\n");
        partie.tour = 1 - partie.tour;
    }

    afficher_resultats(&partie);
    liberer_plateau(partie.plateau, partie.nb_cases);
    return 0;
}



// ============================
//     INITIALISATION
// ============================

void initialiser_partie(Partie* partie) {
    partie->nb_cases = 16;
    partie->plateau = creer_plateau(partie->nb_cases);
    partie->scores[0] = partie->scores[1] = 0;
    partie->tour = 0; // humain commence
    initialiser_killer_moves();
}



// ============================
//       TOUR DE JEU
// ============================

int jouer_tour(Partie* partie) {
    int joueur = partie->tour + 1;
    char* noms[2] = {"Humain (classique)", "IA (famine)"};
    printf("\n=== Tour de %s ===\n", noms[partie->tour]);

    int best_case = -1, best_color = -1, best_mode = 0;

    if (joueur == 1) {
        // --- Tour HUMAIN ---
        printf("👉 Entrez votre coup (ex: 1R, 5B, 3TR, 7TB) : ");
        char input[8];
        if (scanf("%7s", input) != 1) return 0;

        int c = 0, col = 0, mode = 0;

        // --- Extraction du numéro de case ---
        sscanf(input, "%d", &c);

        // --- Recherche des lettres après le chiffre ---
        char* p = input;
        while (*p && (*p >= '0' && *p <= '9')) p++;  // avance jusqu’à la première lettre

        if (*p) {
            if (*p == 'R') { col = 1; mode = 0; }
            else if (*p == 'B') { col = 2; mode = 0; }
            else if (*p == 'T') {
                col = 3;
                p++;
                if (*p == 'R') mode = 1;
                else if (*p == 'B') mode = 2;
            }
        }

        // --- Vérification simple ---
        if (c < 1 || c > partie->nb_cases || col == 0) {
            printf("❌ Coup invalide. Exemples valides : 1R, 5B, 2TR, 3TB.\n");
            return 1; // on redonne la main
        }

        Plateau* case_j = trouver_case(partie->plateau, c);
        Plateau* derniere = distribuer(case_j, col, joueur, mode);
        int captures = capturer(partie->plateau, derniere, partie->nb_cases);
        partie->scores[0] += captures;

        printf("✅ Tu joues %d%s et captures %d graines.\n",
               c, (col == 1 ? "R" : col == 2 ? "B" : mode == 1 ? "TR" : "TB"), captures);
    }
    else {
        // --- Tour IA ---
        printf("🤖 L’IA calcule son coup...\n");
        int ok = choisir_meilleur_coup(partie, joueur, &best_case, &best_color, &best_mode);
        if (!ok) {
            printf("⚠️  Aucun coup possible pour l’IA.\n");
            return 0;
        }
        executer_coup(partie, joueur, best_case, best_color, best_mode);
    }

    afficher_plateau(partie->plateau, partie->nb_cases);
    printf("Scores -> Humain: %d | IA: %d\n", partie->scores[0], partie->scores[1]);
    return 1;
}




// ============================
//      IA (meilleur coup)
// ============================

int choisir_meilleur_coup(Partie* partie, int joueur, int* best_case, int* best_color, int* best_mode) {
    const int nb_cases = partie->nb_cases;
    Plateau* plateau = partie->plateau;

    struct timeval debut;
    gettimeofday(&debut, NULL);

    omp_set_nested(0);
    omp_set_dynamic(0);

    #ifdef _OPENMP
    #pragma omp parallel
    {
        initialiser_killer_moves();
    }
    #endif

    double duree_derniere = 0.0;
    int profondeur_finale = 1;
    Move best_previous = {-1, -1, 0};
    int last_score = 0;
    int have_last = 0;

    for (int d = 1; d <= 25; d++) {

        Move moves[16 * 3 * 2];
        int nmoves = 0;
        Plateau* p = plateau;
        for (int i = 0; i < nb_cases; i++) {
            if (case_du_joueur(p->caseN, joueur)) {
                if (p->R > 0) moves[nmoves++] = (Move){p->caseN, 1, 0};
                if (p->B > 0) moves[nmoves++] = (Move){p->caseN, 2, 0};
                if (p->T > 0) {
                    moves[nmoves++] = (Move){p->caseN, 3, 1};
                    moves[nmoves++] = (Move){p->caseN, 3, 2};
                }
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

            #pragma omp parallel for schedule(dynamic,1) reduction(max:meilleur_score_local)
            for (int k = 0; k < nmoves; k++) {
                Move mv = moves[k];
                Plateau* copie = copier_plateau(plateau, nb_cases);
                Plateau* case_copie = trouver_case(copie, mv.caseN);
                Plateau* derniere = distribuer(case_copie, mv.couleur, joueur,
                                               (mv.couleur == 3 ? mv.mode : 0));
                int captures = capturer(copie, derniere, nb_cases);

                int score = minimax(copie, joueur, d, alpha_root, beta_root, 1) + captures * 5;

                #pragma omp critical
                {
                    if (score > meilleur_score_local) {
                        meilleur_score_local = score;
                        best_case_temp  = mv.caseN;
                        best_color_temp = mv.couleur;
                        best_mode_temp  = (mv.couleur == 3 ? mv.mode : 0);
                    }
                }
                liberer_plateau(copie, nb_cases);
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



// ============================
//      EXECUTION + FIN
// ============================

void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode) {
    char* noms[2] = {"Humain (classique)", "IA (famine)"};
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
        printf("💀 Famine ! IA récupère toutes les graines restantes (%d).\n", totalJ2);
        partie->scores[1] += totalJ2;
        return 1;
    }
    if (totalJ2 == 0 && totalJ1 > 0) {
        printf("💀 Famine ! Tu récupères toutes les graines restantes (%d).\n", totalJ1);
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
    printf("Score final -> Toi: %d | IA: %d\n", partie->scores[0], partie->scores[1]);
    if (partie->scores[0] > partie->scores[1])
        printf("🏆 Bravo ! Tu gagnes 🎉\n");
    else if (partie->scores[1] > partie->scores[0])
        printf("🤖 Victoire de l’IA famine.\n");
    else
        printf("🤝 Match nul.\n");
}
