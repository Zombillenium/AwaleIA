#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>

#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"

#define TEMPS_MAX 0.5  // Temps max par coup (en secondes)

// --- Fonctions ---
void initialiser_partie(Partie* partie);
int jouer_tour(Partie* partie);
void afficher_resultats(Partie* partie);
void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode);

// ============================
//        PROGRAMME PRINCIPAL
// ============================

int main() {

    initialiser_zobrist();
    initialiser_transpo_lock();
    initialiser_killer_moves();

    printf("💻 OpenMP détecte %d threads disponibles.\n", omp_get_max_threads());

    Partie partie;
    initialiser_partie(&partie);

    printf("=== Début du match IA (classique) vs IA (famine) ===\n");
    afficher_plateau(partie.plateau);

    while (1) {
        if (!jouer_tour(&partie)) break;              // Fin si aucun coup possible
        if (verifier_conditions_fin(&partie)) break;  // Fin de partie centralisée dans jeu.c

        printf("----------------------------------------\n");
        partie.tour = 1 - partie.tour;
    }

    afficher_resultats(&partie);
    liberer_plateau(partie.plateau);
    return 0;
}

// ============================
//       INITIALISATION
// ============================

void initialiser_partie(Partie* partie) {
    partie->plateau = creer_plateau();   // plus de nb_cases
    partie->scores[0] = partie->scores[1] = 0;
    partie->tour = 0;
}

// ============================
//       TOUR DE JEU
// ============================

int jouer_tour(Partie* partie) {

    int joueur = partie->tour + 1;
    char* noms[2] = {"J1 (classique)", "J2 (famine)"};

    printf("\n=== Tour de %s ===\n", noms[partie->tour]);

    int best_case = -1;
    int best_color = -1;
    int best_mode = 0;

    int ok = choisir_meilleur_coup(partie, joueur, &best_case, &best_color, &best_mode);

    if (!ok) {
        printf("⚠️  Aucun coup possible pour %s\n", noms[partie->tour]);
        return 0;
    }

    executer_coup(partie, joueur, best_case, best_color, best_mode);
    afficher_plateau(partie->plateau);

    printf("Scores -> J1: %d | J2: %d\n", partie->scores[0], partie->scores[1]);

    int evalJ1 = evaluer_plateau(partie->plateau, 1);
    int evalJ2 = evaluer_plateau_famine(partie->plateau, 2);

    printf("Évaluation -> J1: %d | J2: %d\n", evalJ1, evalJ2);
    return 1;
}

// ============================
//   EXECUTION DU COUP
// ============================

void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode) {

    char* noms[2] = {"J1 (classique)", "J2 (famine)"};
    Plateau* c = trouver_case(partie->plateau, best_case);

    Plateau* derniere = distribuer(c, best_color, joueur, best_mode);
    int captures = capturer(partie->plateau, derniere);

    partie->scores[joueur - 1] += captures;

    printf("👉 %s joue ", noms[joueur - 1]);
    if (best_color == 1) printf("%dR", best_case);
    else if (best_color == 2) printf("%dB", best_case);
    else if (best_color == 3 && best_mode == 1) printf("%dTR", best_case);
    else if (best_color == 3 && best_mode == 2) printf("%dTB", best_case);

    printf(" et capture %d graines.\n", captures);
}

// ============================
//        RESULTATS
// ============================

void afficher_resultats(Partie* partie) {

    printf("\n=== Fin de la partie ===\n");
    printf("Score final -> J1: %d | J2: %d\n",
           partie->scores[0], partie->scores[1]);

    if (partie->scores[0] > partie->scores[1])
        printf("🏆 Victoire de J1 (classique)\n");
    else if (partie->scores[1] > partie->scores[0])
        printf("🏆 Victoire de J2 (famine)\n");
    else
        printf("🤝 Match nul\n");
}
