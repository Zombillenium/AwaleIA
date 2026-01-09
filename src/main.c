#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>

#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"

// Types d'IA
#define TYPE_IA_CLASSIQUE 1
#define TYPE_IA_FAMINE 2
#define TYPE_IA_MULTI_PHASES 3

// --- Fonctions ---
void initialiser_partie(Partie* partie);
int jouer_tour(Partie* partie, int type_j1, int type_j2);
void afficher_resultats(Partie* partie, int type_j1, int type_j2, int nb_coups, int nb_coups_max);
void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode, int type_j1, int type_j2);
void afficher_aide(void);
const char* nom_type_ia(int type);
void afficher_menu_configuration(int* nb_coups_max, int* type_j1, int* type_j2);

// ============================
//        PROGRAMME PRINCIPAL
// ============================

int main(int argc, char* argv[]) {
    // Valeurs par défaut
    int nb_coups_max = 200;
    int type_j1 = TYPE_IA_CLASSIQUE;
    int type_j2 = TYPE_IA_FAMINE;

    // Si des arguments sont fournis, les utiliser (mode non-interactif)
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                afficher_aide();
                return 0;
            }
            else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--max-coups") == 0) {
                if (i + 1 < argc) {
                    nb_coups_max = atoi(argv[++i]);
                    if (nb_coups_max <= 0) {
                        fprintf(stderr, "❌ Erreur: le nombre de coups maximum doit être positif\n");
                        return 1;
                    }
                } else {
                    fprintf(stderr, "❌ Erreur: -n/--max-coups nécessite un argument\n");
                    return 1;
                }
            }
            else if (strcmp(argv[i], "-j1") == 0 || strcmp(argv[i], "--joueur1") == 0) {
                if (i + 1 < argc) {
                    int type = atoi(argv[++i]);
                    if (type < 1 || type > 2) {
                        fprintf(stderr, "❌ Erreur: type d'IA invalide pour J1 (doit être 1 ou 2)\n");
                        return 1;
                    }
                    type_j1 = type;
                } else {
                    fprintf(stderr, "❌ Erreur: -j1/--joueur1 nécessite un argument\n");
                    return 1;
                }
            }
            else if (strcmp(argv[i], "-j2") == 0 || strcmp(argv[i], "--joueur2") == 0) {
                if (i + 1 < argc) {
                    int type = atoi(argv[++i]);
                    if (type < 1 || type > 2) {
                        fprintf(stderr, "❌ Erreur: type d'IA invalide pour J2 (doit être 1 ou 2)\n");
                        return 1;
                    }
                    type_j2 = type;
                } else {
                    fprintf(stderr, "❌ Erreur: -j2/--joueur2 nécessite un argument\n");
                    return 1;
                }
            }
            else {
                fprintf(stderr, "❌ Erreur: argument inconnu '%s'\n", argv[i]);
                fprintf(stderr, "Utilisez -h ou --help pour afficher l'aide\n");
                return 1;
            }
        }
    } else {
        // Mode interactif : afficher le menu de configuration
        afficher_menu_configuration(&nb_coups_max, &type_j1, &type_j2);
    }

    initialiser_zobrist();
    initialiser_transpo_lock();
    initialiser_killer_moves();

    printf("💻 OpenMP détecte %d threads disponibles.\n", omp_get_max_threads());

    Partie partie;
    initialiser_partie(&partie);
    
    // Compteur de coups local
    int nb_coups = 0;
    
    // Configurer les types d'IA APRÈS l'initialisation (pour ne pas être réinitialisés)
    definir_type_ia(1, type_j1);
    definir_type_ia(2, type_j2);

    printf("=== Début du match IA (%s) vs IA (%s) ===\n", nom_type_ia(type_j1), nom_type_ia(type_j2));
    printf("Limite de coups : %d\n", nb_coups_max);
    afficher_plateau(partie.plateau);

    while (1) {
        if (!jouer_tour(&partie, type_j1, type_j2)) break;              // Fin si aucun coup possible
        nb_coups++;                             // Incrémenter le compteur de coups
        if (verifier_conditions_fin(&partie)) break;  // Fin de partie centralisée dans jeu.c
        if (nb_coups_max > 0 && nb_coups >= nb_coups_max) break;  // Limite de coups atteinte

        printf("----------------------------------------\n");
        partie.tour = 1 - partie.tour;
    }

    afficher_resultats(&partie, type_j1, type_j2, nb_coups, nb_coups_max);
    liberer_plateau(partie.plateau);
    return 0;
}

// ============================
//       INITIALISATION
// ============================

void initialiser_partie(Partie* partie) {
    partie->plateau = creer_plateau();   // plus de nb_cases
    if (!partie->plateau) {
        printf("❌ Erreur: impossible de créer le plateau\n");
        exit(1);
    }
    partie->scores[0] = partie->scores[1] = 0;
    partie->tour = 0;
    // Note: reinitialiser_type_ia() n'est pas appelé ici car les types sont configurés dans main()
}

// Fonction utilitaire pour obtenir le nom d'un type d'IA
const char* nom_type_ia(int type) {
    switch (type) {
        case TYPE_IA_CLASSIQUE: return "Classique";
        case TYPE_IA_FAMINE: return "Famine";
        default: return "Inconnu";
    }
}

// Fonction d'aide
void afficher_aide(void) {
    printf("Usage: ./bin/main [OPTIONS]\n\n");
    printf("Options:\n");
    printf("  -n, --max-coups <nombre>    Nombre maximum de coups (défaut: 200)\n");
    printf("  -j1, --joueur1 <type>       Type d'IA pour J1 (1=Classique, 2=Famine, défaut: 1)\n");
    printf("  -j2, --joueur2 <type>       Type d'IA pour J2 (1=Classique, 2=Famine, défaut: 2)\n");
    printf("  -h, --help                   Afficher cette aide\n\n");
    printf("Exemples:\n");
    printf("  ./bin/main -n 400 -j1 1 -j2 2    # 400 coups, Classique vs Famine\n");
    printf("  ./bin/main --max-coups 100        # 100 coups, Classique vs Famine (défaut)\n");
    printf("  ./bin/main -j1 2 -j2 1           # 200 coups (défaut), Famine vs Classique\n");
    printf("\n");
    printf("Note: Si aucun argument n'est fourni, un menu interactif sera affiché.\n");
}

// Menu de configuration interactif
void afficher_menu_configuration(int* nb_coups_max, int* type_j1, int* type_j2) {
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║        CONFIGURATION DE LA PARTIE                      ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
    
    // Premier choix : nombre de coups maximum
    printf("Nombre maximum de coups pour cette partie ?\n");
    printf("  (Entrez un nombre positif, ou 0 pour illimité, défaut: 200) : ");
    int choix;
    int resultat_scan = scanf("%d", &choix);
    // Vider le buffer en cas d'erreur
    if (resultat_scan != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        printf("Choix invalide, utilisation de 200 par défaut.\n");
        *nb_coups_max = 200;
    } else if (choix < 0) {
        printf("Le nombre de coups ne peut pas être négatif, utilisation de 200 par défaut.\n");
        *nb_coups_max = 200;
    } else {
        *nb_coups_max = choix;
    }
    
    // Deuxième choix : type d'IA pour J1
    printf("\nType d'IA pour J1 (qui joue en premier) ?\n");
    printf("  1) IA Classique\n");
    printf("  2) IA Famine\n");
    printf("Votre choix (1 ou 2, défaut: 1) : ");
    resultat_scan = scanf("%d", &choix);
    // Vider le buffer en cas d'erreur
    if (resultat_scan != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        printf("Choix invalide, utilisation de l'IA Classique par défaut.\n");
        *type_j1 = TYPE_IA_CLASSIQUE;
    } else if (choix < 1 || choix > 2) {
        printf("Choix invalide (doit être 1 ou 2), utilisation de l'IA Classique par défaut.\n");
        *type_j1 = TYPE_IA_CLASSIQUE;
    } else {
        *type_j1 = choix;
    }
    
    // Troisième choix : type d'IA pour J2
    printf("\nType d'IA pour J2 (qui joue en deuxième) ?\n");
    printf("  1) IA Classique\n");
    printf("  2) IA Famine\n");
    printf("Votre choix (1 ou 2, défaut: 2) : ");
    resultat_scan = scanf("%d", &choix);
    // Vider le buffer en cas d'erreur
    if (resultat_scan != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        printf("Choix invalide, utilisation de l'IA Famine par défaut.\n");
        *type_j2 = TYPE_IA_FAMINE;
    } else if (choix < 1 || choix > 2) {
        printf("Choix invalide (doit être 1 ou 2), utilisation de l'IA Famine par défaut.\n");
        *type_j2 = TYPE_IA_FAMINE;
    } else {
        *type_j2 = choix;
    }
    
    printf("\n✅ Configuration finale :\n");
    if (*nb_coups_max == 0) {
        printf("   - Nombre de coups maximum : Illimité\n");
    } else {
        printf("   - Nombre de coups maximum : %d\n", *nb_coups_max);
    }
    printf("   - J1 (en premier) : %s\n", nom_type_ia(*type_j1));
    printf("   - J2 (en deuxième) : %s\n", nom_type_ia(*type_j2));
    printf("\n");
}

// ============================
//       TOUR DE JEU
// ============================

int jouer_tour(Partie* partie, int type_j1, int type_j2) {

    int joueur = partie->tour + 1;
    char nom_j1[64], nom_j2[64];
    snprintf(nom_j1, sizeof(nom_j1), "J1 (%s)", nom_type_ia(type_j1));
    snprintf(nom_j2, sizeof(nom_j2), "J2 (%s)", nom_type_ia(type_j2));
    char* noms[2] = {nom_j1, nom_j2};

    // Vérification de sécurité pour partie->tour
    if (partie->tour < 0 || partie->tour > 1) {
        printf("⚠️  Erreur: tour invalide (%d)\n", partie->tour);
        return 0;
    }
    
    printf("\n=== Tour de %s ===\n", noms[partie->tour]);

    int best_case = -1;
    int best_color = -1;
    int best_mode = 0;

    int ok = choisir_meilleur_coup(partie, joueur, &best_case, &best_color, &best_mode);

    if (!ok) {
        printf("⚠️  Aucun coup possible pour %s\n", noms[partie->tour]);
        return 0;
    }

    executer_coup(partie, joueur, best_case, best_color, best_mode, type_j1, type_j2);
    afficher_plateau(partie->plateau);

    printf("Scores -> J1: %d | J2: %d\n", partie->scores[0], partie->scores[1]);

    // Évaluations selon les types d'IA (pour affichage uniquement)
    int evalJ1, evalJ2;
    if (type_j1 == TYPE_IA_CLASSIQUE) {
        evalJ1 = evaluer_plateau_avance(partie->plateau, 1);
    } else { // TYPE_IA_FAMINE
        evalJ1 = evaluer_plateau_famine_avance(partie->plateau, 1);
    }
    
    if (type_j2 == TYPE_IA_CLASSIQUE) {
        evalJ2 = evaluer_plateau_avance(partie->plateau, 2);
    } else { // TYPE_IA_FAMINE
        evalJ2 = evaluer_plateau_famine_avance(partie->plateau, 2);
    }

    printf("Évaluation -> J1: %d | J2: %d\n", evalJ1, evalJ2);
    return 1;
}

// ============================
//   EXECUTION DU COUP
// ============================

void executer_coup(Partie* partie, int joueur, int best_case, int best_color, int best_mode, int type_j1, int type_j2) {
    char nom_j1[64], nom_j2[64];
    snprintf(nom_j1, sizeof(nom_j1), "J1 (%s)", nom_type_ia(type_j1));
    snprintf(nom_j2, sizeof(nom_j2), "J2 (%s)", nom_type_ia(type_j2));
    char* noms[2] = {nom_j1, nom_j2};
    
    Plateau* c = trouver_case(partie->plateau, best_case);
    if (!c) {
        printf("❌ Erreur: case %d introuvable\n", best_case);
        return;
    }

    Plateau* derniere = distribuer(c, best_color, joueur, best_mode);
    if (!derniere) {
        printf("❌ Erreur lors de la distribution des graines\n");
        return;
    }
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

void afficher_resultats(Partie* partie, int type_j1, int type_j2, int nb_coups, int nb_coups_max) {
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║              STATISTIQUES DE LA PARTIE                ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
    
    // Nombre de coups
    printf("📊 Nombre de coups joués : %d", nb_coups);
    if (nb_coups_max > 0 && nb_coups >= nb_coups_max) {
        printf(" (limite de %d coups atteinte)", nb_coups_max);
    }
    printf("\n");
    
    // Scores finaux
    printf("🎯 Scores finaux :\n");
    printf("   J1 (%s) : %d points\n", nom_type_ia(type_j1), partie->scores[0]);
    printf("   J2 (%s) : %d points\n", nom_type_ia(type_j2), partie->scores[1]);
    printf("   Écart : %d points\n", abs(partie->scores[0] - partie->scores[1]));
    
    // Graines restantes sur le plateau
    int graines_j1 = total_graines_joueur(partie->plateau, 1);
    int graines_j2 = total_graines_joueur(partie->plateau, 2);
    int graines_total = total_graines(partie->plateau);
    
    printf("\n🌾 Graines restantes sur le plateau :\n");
    printf("   J1 : %d graines\n", graines_j1);
    printf("   J2 : %d graines\n", graines_j2);
    printf("   Total : %d graines\n", graines_total);
    
    // Phase de jeu
    float phase = calculer_phase_jeu(partie->plateau);
    printf("   Phase de jeu : %.1f%% (0%% = début, 100%% = fin)\n", phase * 100.0f);
    
    // Scores totaux (captures + graines restantes)
    int total_j1 = partie->scores[0] + graines_j1;
    int total_j2 = partie->scores[1] + graines_j2;
    printf("\n💎 Scores totaux (captures + graines restantes) :\n");
    printf("   J1 : %d points\n", total_j1);
    printf("   J2 : %d points\n", total_j2);
    
    // Moyenne de captures par coup
    if (nb_coups > 0) {
        float moy_captures_j1 = (float)partie->scores[0] / nb_coups;
        float moy_captures_j2 = (float)partie->scores[1] / nb_coups;
        printf("\n📈 Moyenne de captures par coup :\n");
        printf("   J1 : %.2f graines/coup\n", moy_captures_j1);
        printf("   J2 : %.2f graines/coup\n", moy_captures_j2);
    }
    
    // Résultat final
    printf("\n🏁 Résultat final :\n");
    if (partie->scores[0] > partie->scores[1])
        printf("   🏆 Victoire de J1 (%s) !\n", nom_type_ia(type_j1));
    else if (partie->scores[1] > partie->scores[0])
        printf("   🏆 Victoire de J2 (%s) !\n", nom_type_ia(type_j2));
    else
        printf("   🤝 Match nul\n");
    
    printf("\n");
}
