#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>

#include "evaluation.h"
#include "plateau.h"
#include "jeu.h"
#include "ia.h"
#include "tabletranspo.h"

#define TEMPS_MAX 2.0  // Temps max par coup (en secondes) - sous le timeout de 3s de l'arbitre
#define MAX_COUPS 400  // Limite de 400 coups au total

// Types d'IA (compatibles avec main.c)
#define TYPE_IA_CLASSIQUE 1
#define TYPE_IA_FAMINE 2
#define TYPE_IA_MULTI_PHASES 3

// Variables globales
static int mon_joueur = 1;  // 1 = J1, 2 = J2 (sera déterminé au début)
static int type_ia = TYPE_IA_MULTI_PHASES;  // Type d'IA à utiliser

// --- Fonctions ---
int parser_coup(const char* input, int* case_num, int* couleur, int* mode);
int appliquer_coup_adverse(Partie* partie, int case_num, int couleur, int mode);
void formater_coup(int case_num, int couleur, int mode, char* output);

// ============================
//        PROGRAMME PRINCIPAL
// ============================

int main() {
    // Initialisation des structures
    initialiser_zobrist();
    initialiser_transpo_lock();
    initialiser_killer_moves();

    // Créer et initialiser la partie
    Partie partie;
    partie.plateau = creer_plateau();
    if (!partie.plateau) {
        fprintf(stderr, "Erreur: impossible de créer le plateau\n");
        return 1;
    }
    partie.scores[0] = 0;
    partie.scores[1] = 0;
    partie.tour = 0;  // 0 = J1, 1 = J2

    // Compteur de coups local (limite de 400 coups au total)
    int nb_coups = 0;

    // Configurer le type d'IA
    definir_type_ia(1, type_ia);
    definir_type_ia(2, type_ia);

    // Boucle principale de communication avec l'arbitre
    char ligne[256];
    int premier_tour = 1;
    char dernier_coup_joue[8] = "";

    while (1) {
        // Lire depuis stdin
        if (fgets(ligne, sizeof(ligne), stdin) == NULL) {
            break;  // Fin de l'entrée
        }

        // Supprimer le saut de ligne
        size_t len = strlen(ligne);
        if (len > 0 && ligne[len - 1] == '\n') {
            ligne[len - 1] = '\0';
        }

        // Traiter le message "START"
        if (strcmp(ligne, "START") == 0) {
            // On joue en premier, on est J1
            mon_joueur = 1;
            partie.tour = 0;
            premier_tour = 0;
            
            // Calculer et envoyer notre premier coup
            int best_case = -1, best_color = -1, best_mode = 0;
            int ok = choisir_meilleur_coup(&partie, mon_joueur, &best_case, &best_color, &best_mode);
            
            if (!ok) {
                // Aucun coup possible
                printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
                fflush(stdout);
                break;
            }

            // Formater et envoyer le coup
            formater_coup(best_case, best_color, best_mode, dernier_coup_joue);
            printf("%s\n", dernier_coup_joue);
            fflush(stdout);

            // Appliquer notre coup au plateau
            Plateau* c = trouver_case(partie.plateau, best_case);
            Plateau* derniere = distribuer(c, best_color, mon_joueur, best_mode);
            int captures = capturer(partie.plateau, derniere);
            partie.scores[mon_joueur - 1] += captures;
            nb_coups++;

            // Vérifier fin de partie
            if (nb_coups >= MAX_COUPS) {
                printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
                fflush(stdout);
                break;
            }

            if (verifier_conditions_fin(&partie)) {
                printf("RESULT %s %d %d\n", dernier_coup_joue, partie.scores[0], partie.scores[1]);
                fflush(stdout);
                break;
            }

            partie.tour = 1 - partie.tour;
            continue;
        }

        // Parser le coup de l'adversaire
        int case_num = 0, couleur = 0, mode = 0;
        if (!parser_coup(ligne, &case_num, &couleur, &mode)) {
            // Coup invalide - envoyer RESULT avec disqualification
            printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        // Déterminer le joueur adverse si c'est le premier coup reçu
        if (premier_tour) {
            // Si on reçoit un coup, on est J2 (l'adversaire a joué en premier)
            mon_joueur = 2;
            partie.tour = 0;  // L'adversaire (J1) vient de jouer, on met le tour à J1
            premier_tour = 0;
        }

        // Appliquer le coup de l'adversaire
        if (!appliquer_coup_adverse(&partie, case_num, couleur, mode)) {
            // Coup invalide
            printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        nb_coups++;

        // Vérifier fin de partie après le coup adverse
        if (nb_coups >= MAX_COUPS) {
            printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        if (verifier_conditions_fin(&partie)) {
            printf("RESULT %s %d %d\n", dernier_coup_joue, partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        // Alterner le tour (l'adversaire vient de jouer, c'est maintenant notre tour)
        partie.tour = (mon_joueur == 1) ? 0 : 1;  // Si on est J1, tour=0 (J1), sinon tour=1 (J2)

        // Calculer et envoyer notre coup
        int best_case = -1, best_color = -1, best_mode = 0;
        int ok = choisir_meilleur_coup(&partie, mon_joueur, &best_case, &best_color, &best_mode);
        
        if (!ok) {
            // Aucun coup possible
            printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        // Formater et envoyer le coup
        formater_coup(best_case, best_color, best_mode, dernier_coup_joue);
        printf("%s\n", dernier_coup_joue);
        fflush(stdout);

        // Appliquer notre coup au plateau
        Plateau* c = trouver_case(partie.plateau, best_case);
        Plateau* derniere = distribuer(c, best_color, mon_joueur, best_mode);
        int captures = capturer(partie.plateau, derniere);
        partie.scores[mon_joueur - 1] += captures;
        nb_coups++;

        // Vérifier fin de partie après notre coup
        if (nb_coups >= MAX_COUPS) {
            printf("RESULT LIMIT %d %d\n", partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        if (verifier_conditions_fin(&partie)) {
            printf("RESULT %s %d %d\n", dernier_coup_joue, partie.scores[0], partie.scores[1]);
            fflush(stdout);
            break;
        }

        // Alterner le tour (on vient de jouer, c'est maintenant le tour de l'adversaire)
        partie.tour = (mon_joueur == 1) ? 1 : 0;  // Si on est J1, tour=1 (J2), sinon tour=0 (J1)
    }

    liberer_plateau(partie.plateau);
    return 0;
}

// ============================
//       PARSING DU COUP
// ============================

int parser_coup(const char* input, int* case_num, int* couleur, int* mode) {
    if (!input || !case_num || !couleur || !mode) {
        return 0;
    }

    *case_num = 0;
    *couleur = 0;
    *mode = 0;

    // Extraction du numéro de case
    if (sscanf(input, "%d", case_num) != 1) {
        return 0;
    }

    // Recherche des lettres après le chiffre
    const char* p = input;
    while (*p && (*p >= '0' && *p <= '9')) {
        p++;
    }

    if (!*p) {
        return 0;  // Pas de couleur spécifiée
    }

    // Détection de la couleur
    if (*p == 'R') {
        *couleur = 1;  // Rouge
        *mode = 0;
    } else if (*p == 'B') {
        *couleur = 2;  // Bleu
        *mode = 0;
    } else if (*p == 'T') {
        *couleur = 3;  // Transparent
        p++;
        if (*p == 'R') {
            *mode = 1;  // TR
        } else if (*p == 'B') {
            *mode = 2;  // TB
        } else {
            return 0;  // Format invalide pour T
        }
    } else {
        return 0;  // Couleur invalide
    }

    // Vérifications de base
    if (*case_num < 1 || *case_num > nb_cases) {
        return 0;
    }

    return 1;
}

// ============================
//   APPLICATION COUP ADVERSE
// ============================

int appliquer_coup_adverse(Partie* partie, int case_num, int couleur, int mode) {
    if (!partie || !partie->plateau) {
        return 0;
    }

    // Trouver la case
    Plateau* case_j = trouver_case(partie->plateau, case_num);
    if (!case_j) {
        return 0;
    }

    // Déterminer le joueur adverse
    int joueur_adverse = (mon_joueur == 1) ? 2 : 1;

    // Vérifier que la case appartient au joueur adverse
    if (!case_du_joueur(case_num, joueur_adverse)) {
        return 0;
    }

    // Vérifier qu'il y a des graines disponibles
    int graines_disponibles = 0;
    if (couleur == 1) {
        graines_disponibles = case_j->R;
    } else if (couleur == 2) {
        graines_disponibles = case_j->B;
    } else if (couleur == 3) {
        if (mode == 1) {
            graines_disponibles = case_j->T + case_j->R;  // TR
        } else if (mode == 2) {
            graines_disponibles = case_j->T + case_j->B;  // TB
        }
    }

    if (graines_disponibles == 0) {
        return 0;
    }

    // Appliquer le coup
    Plateau* derniere = distribuer(case_j, couleur, joueur_adverse, mode);
    if (!derniere) {
        return 0;
    }

    int captures = capturer(partie->plateau, derniere);
    partie->scores[joueur_adverse - 1] += captures;

    return 1;
}

// ============================
//    FORMATAGE DU COUP
// ============================

void formater_coup(int case_num, int couleur, int mode, char* output) {
    if (!output) {
        return;
    }

    if (couleur == 1) {
        sprintf(output, "%dR", case_num);
    } else if (couleur == 2) {
        sprintf(output, "%dB", case_num);
    } else if (couleur == 3) {
        if (mode == 1) {
            sprintf(output, "%dTR", case_num);
        } else if (mode == 2) {
            sprintf(output, "%dTB", case_num);
        } else {
            sprintf(output, "%dT", case_num);  // Par défaut
        }
    } else {
        output[0] = '\0';
    }
}

