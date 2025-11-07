#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"
#include <omp.h>


#define TEMPS_MAX 2.0  // Temps max par coup (en secondes)

// --- Déclarations ---
int total_graines(Plateau* plateau, int nb_cases);
int total_graines_joueur(Plateau* plateau, int nb_cases, int joueur);


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
