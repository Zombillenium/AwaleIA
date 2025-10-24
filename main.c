#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"
#include <omp.h>


#define TEMPS_MAX 0.5  // Temps max par coup (en secondes)

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
    const int nb_cases = partie->nb_cases;
    Plateau* plateau = partie->plateau;

    struct timeval debut, courant;
    gettimeofday(&debut, NULL); // chrono global

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

    // <<< ASPIRATION : on mémorise le score racine précédent
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

        // <<< iterative deepening : prioriser le meilleur coup précédent
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

        // <<< ASPIRATION : fenêtre initiale
        int ASP = 75;
        int alpha_root = have_last ? (last_score - ASP) : -100000;
        int beta_root  = have_last ? (last_score + ASP) :  100000;

        int best_case_temp = -1, best_color_temp = -1, best_mode_temp = 0;
        int meilleur_score_local = -999999;

        // On va potentiellement relancer la même profondeur si la fenêtre est trop serrée
        for (;;) { // boucle d'aspiration
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

                // <<< ASPIRATION : on utilise alpha/beta racine serrés
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

            // Affichage standard
            printf("🔹 Profondeur %d terminée en %.3fs | Meilleur coup : ", d, duree);
            if (best_color_temp == 1) printf("%dR", best_case_temp);
            else if (best_color_temp == 2) printf("%dB", best_case_temp);
            else if (best_color_temp == 3 && best_mode_temp == 1) printf("%dTR", best_case_temp);
            else if (best_color_temp == 3 && best_mode_temp == 2) printf("%dTB", best_case_temp);
            printf(" (score %d)", meilleur_score_local);

            // <<< ASPIRATION : détection fail-low / fail-high
            if (have_last && meilleur_score_local <= alpha_root) {
                // fail-low → élargir vers le bas et relancer
                ASP *= 2;
                alpha_root = last_score - ASP;
                beta_root  = last_score + ASP;
                printf("  ⤵️  fail-low → élargit à [%d, %d]\n", alpha_root, beta_root);
                fflush(stdout);
                continue; // relance la profondeur d
            } else if (have_last && meilleur_score_local >= beta_root) {
                // fail-high → élargir vers le haut et relancer
                ASP *= 2;
                alpha_root = last_score - ASP;
                beta_root  = last_score + ASP;
                printf("  ⤴️  fail-high → élargit à [%d, %d]\n", alpha_root, beta_root);
                fflush(stdout);
                continue; // relance la profondeur d
            } else {
                printf("\n");
                break; // fenêtre OK, on valide cette profondeur
            }
        }

        // On valide les résultats pour cette profondeur
        *best_case = best_case_temp;
        *best_color = best_color_temp;
        *best_mode = best_mode_temp;
        profondeur_finale = d;
        best_previous = (Move){*best_case, *best_color, *best_mode};

        // <<< ASPIRATION : mémorise le score racine
        last_score = (meilleur_score_local == -999999) ? 0 : meilleur_score_local;
        have_last = 1;

        // Stop si on va dépasser 2s
        if (duree_derniere * 8.0 >= TEMPS_MAX) {
            printf("⏱️  Prochaine profondeur estimée à %.3fs (>%.1fs) → arrêt anticipé.\n",
                   duree_derniere * 8.0, TEMPS_MAX);
            break;
        }
    }

    printf("✅ Profondeur finale retenue : %d\n", profondeur_finale);
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
