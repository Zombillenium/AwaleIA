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

#define TEMPS_MAX 2.00  // Temps max par coup (en secondes)

// --- Fonctions ---
void initialiser_partie(Partie* partie);
int jouer_tour(Partie* partie);
void afficher_resultats(Partie* partie);
int verifier_conditions_fin(Partie* partie);
void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode);


// ============================
//        PROGRAMME PRINCIPAL
// ============================

int main() {
    initialiser_zobrist();
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
        int coup_valide = 0;
        while (!coup_valide) {
            printf("👉 Entrez votre coup (ex: 1R, 5B, 3TR, 7TB) : ");
            char input[8];
            if (scanf("%7s", input) != 1) return 0;

            int c = 0, col = 0, mode = 0;

            // --- Extraction du numéro de case ---
            sscanf(input, "%d", &c);

            // --- Recherche des lettres après le chiffre ---
            char* p = input;
            while (*p && (*p >= '0' && *p <= '9')) p++;

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

            // --- Vérifications ---
            if (c < 1 || c > partie->nb_cases || col == 0) {
                printf("❌ Coup invalide. Exemples valides : 1R, 5B, 2TR, 3TB.\n");
                continue; // redemande le coup
            }

            if (!case_du_joueur(c, joueur)) {
                printf("⚠️  Cette case (%d) appartient à l’adversaire, tu ne peux pas y jouer.\n", c);
                continue; // redemande le coup
            }

            // --- Vérifie qu’il y a bien des graines du bon type ---
            Plateau* case_j = trouver_case(partie->plateau, c);
            int graines_disponibles = 0;

            if (col == 1) graines_disponibles = case_j->R;
            else if (col == 2) graines_disponibles = case_j->B;
            else if (col == 3) {
                if (mode == 1) graines_disponibles = case_j->T + case_j->R; // TR
                else if (mode == 2) graines_disponibles = case_j->T + case_j->B; // TB
            }

            if (graines_disponibles == 0) {
                printf("❌ Cette case (%d) ne contient aucune graine correspondante à ton coup.\n", c);
                continue; // redemande un coup
            }

            // --- Exécution du coup ---
            Plateau* derniere = distribuer(case_j, col, joueur, mode);
            int captures = capturer(partie->plateau, derniere, partie->nb_cases);
            partie->scores[0] += captures;

            printf("✅ Tu joues %d%s et captures %d graines.\n",
                c, (col == 1 ? "R" : col == 2 ? "B" : mode == 1 ? "TR" : "TB"), captures);

            coup_valide = 1; // on sort de la boucle
        }
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
        // J1 (humain) est affamé → J2 (IA) récupère les graines restantes
        printf("💀 Famine ! IA récupère toutes les graines restantes (%d).\n", totalJ2);
        partie->scores[1] += totalJ2;
        return 1;
    }

    if (totalJ2 == 0 && totalJ1 > 0) {
        // J2 (IA) est affamée → J1 (humain) récupère les graines restantes
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
