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

// Variables globales pour la configuration
static int joueur_humain = 1;  // 1 = J1, 2 = J2
static int type_ia_adverse = 2; // 1 = classique, 2 = famine

// --- Fonctions ---
void initialiser_partie(Partie* partie);
int jouer_tour(Partie* partie, int joueur_humain, int type_ia_adverse);
void afficher_resultats(Partie* partie, int joueur_humain);
void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode, int joueur_humain);
void afficher_menu_configuration();


// ============================
//        PROGRAMME PRINCIPAL
// ============================

int main() {
    initialiser_zobrist();
    printf("💻 OpenMP détecte %d threads disponibles.\n", omp_get_max_threads());
    
    // Menu de configuration
    afficher_menu_configuration();
    
    Partie partie;
    initialiser_partie(&partie);
    initialiser_transpo_lock();

    // Afficher la configuration choisie
    char* nom_ia = (type_ia_adverse == 1) ? "classique" : "famine";
    char* nom_humain = (joueur_humain == 1) ? "J1" : "J2";
    char* nom_ia_joueur = (type_ia_adverse == 1) ? "J1" : "J2";
    if (joueur_humain == 1) {
        printf("=== Début du match Humain (%s) vs IA %s (%s) ===\n", 
               nom_humain, nom_ia, nom_ia_joueur);
    } else {
        printf("=== Début du match IA %s (%s) vs Humain (%s) ===\n", 
               nom_ia, nom_ia_joueur, nom_humain);
    }
    afficher_plateau(partie.plateau);

    while (1) {
        if (!jouer_tour(&partie, joueur_humain, type_ia_adverse)) break;
        if (verifier_conditions_fin(&partie)) break;
        printf("----------------------------------------\n");
        partie.tour = 1 - partie.tour;
    }

    afficher_resultats(&partie, joueur_humain);
    liberer_plateau(partie.plateau);
    return 0;
}



// ============================
//     INITIALISATION
// ============================

void afficher_menu_configuration() {
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║        CONFIGURATION DE LA PARTIE                      ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
    
    // Premier choix : ordre de jeu
    printf("Voulez-vous jouer en premier ou en deuxième ?\n");
    printf("  1) En premier (J1)\n");
    printf("  2) En deuxième (J2)\n");
    printf("Votre choix (1 ou 2) : ");
    int choix;
    if (scanf("%d", &choix) != 1 || (choix != 1 && choix != 2)) {
        printf("Choix invalide, utilisation de J1 par défaut.\n");
        joueur_humain = 1;
    } else {
        joueur_humain = choix;
    }
    
    // Deuxième choix : type d'IA adverse
    printf("\nQuel type d'IA voulez-vous affronter ?\n");
    printf("  1) IA Classique\n");
    printf("  2) IA Famine\n");
    printf("Votre choix (1 ou 2) : ");
    if (scanf("%d", &choix) != 1 || (choix != 1 && choix != 2)) {
        printf("Choix invalide, utilisation de l'IA Famine par défaut.\n");
        type_ia_adverse = 2;
    } else {
        type_ia_adverse = choix;
    }
    
    // Déterminer le joueur de l'IA
    int joueur_ia = (joueur_humain == 1) ? 2 : 1;
    
    // Configurer le type d'IA pour l'IA adverse
    definir_type_ia(joueur_ia, type_ia_adverse);
    
    printf("\n✅ Configuration finale :\n");
    printf("   - Vous êtes %s\n", (joueur_humain == 1) ? "J1 (en premier)" : "J2 (en deuxième)");
    printf("   - L'IA adverse est %s (%s)\n",
           (joueur_ia == 1) ? "J1" : "J2",
           (type_ia_adverse == 1) ? "Classique" : "Famine");
    printf("\n");
}

void initialiser_partie(Partie* partie) {
    partie->plateau = creer_plateau();        // plus de nb_cases
    partie->scores[0] = partie->scores[1] = 0;
    partie->tour = 0; // J1 commence toujours
    initialiser_killer_moves();
}



// ============================
//       TOUR DE JEU
// ============================

int jouer_tour(Partie* partie, int joueur_humain, int type_ia_adverse) {
    int joueur = partie->tour + 1;
    
    // Déterminer les noms selon la configuration
    char* noms[2];
    if (joueur == joueur_humain) {
        noms[partie->tour] = "Humain";
    } else {
        noms[partie->tour] = (type_ia_adverse == 1) ? "IA (classique)" : "IA (famine)";
    }
    
    // Si l'autre joueur est aussi humain (ne devrait pas arriver), gérer le cas
    if (joueur != joueur_humain) {
        // C'est l'IA qui joue
    } else {
        // C'est l'humain qui joue
    }
    
    printf("\n=== Tour de %s ===\n", noms[partie->tour]);

    int best_case = -1, best_color = -1, best_mode = 0;

    if (joueur == joueur_humain) {
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
            if (c < 1 || c > nb_cases || col == 0) {
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
            int captures = capturer(partie->plateau, derniere);
            partie->scores[joueur - 1] += captures;

            printf("✅ Tu joues %d%s et captures %d graines.\n",
                c, (col == 1 ? "R" : col == 2 ? "B" : mode == 1 ? "TR" : "TB"), captures);

            coup_valide = 1; // on sort de la boucle
        }
    }

    else {
        // --- Tour IA ---
        printf("🤖 L'IA calcule son coup...\n");
        int ok = choisir_meilleur_coup(partie, joueur, &best_case, &best_color, &best_mode);
        if (!ok) {
            printf("⚠️  Aucun coup possible pour l'IA.\n");
            return 0;
        }
        executer_coup(partie, joueur, best_case, best_color, best_mode, joueur_humain);
    }

    afficher_plateau(partie->plateau);
    // Afficher les scores selon la configuration
    if (joueur_humain == 1) {
        printf("Scores -> Humain: %d | IA: %d\n", partie->scores[0], partie->scores[1]);
    } else {
        printf("Scores -> IA: %d | Humain: %d\n", partie->scores[0], partie->scores[1]);
    }
    return 1;
}


// ============================
//      EXECUTION + FIN
// ============================

void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode, int joueur_humain) {
    char* nom;
    if (joueur == joueur_humain) {
        nom = "Humain";
    } else {
        // Le type d'IA est déterminé par type_ia_adverse, mais pour l'affichage on peut simplifier
        nom = "IA";
    }
    
    Plateau* c = trouver_case(partie->plateau, best_case);
    Plateau* derniere = distribuer(c, best_color, joueur, best_mode);
    int captures = capturer(partie->plateau, derniere);
    partie->scores[joueur - 1] += captures;

    printf("👉 %s joue ", nom);
    if (best_color == 1) printf("%dR", best_case);
    else if (best_color == 2) printf("%dB", best_case);
    else if (best_color == 3 && best_mode == 1) printf("%dTR", best_case);
    else if (best_color == 3 && best_mode == 2) printf("%dTB", best_case);
    printf(" et capture %d graines.\n", captures);
}


void afficher_resultats(Partie* partie, int joueur_humain) {
    printf("\n=== Fin de la partie ===\n");
    int score_humain = partie->scores[joueur_humain - 1];
    int score_ia = partie->scores[2 - joueur_humain];
    
    printf("Score final -> Humain: %d | IA: %d\n", score_humain, score_ia);
    if (score_humain > score_ia)
        printf("🏆 Bravo ! Tu gagnes 🎉\n");
    else if (score_ia > score_humain)
        printf("🤖 Victoire de l'IA.\n");
    else
        printf("🤝 Match nul.\n");
}
