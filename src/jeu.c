#include "jeu.h"
#include <stdlib.h>

int capturer(Plateau* plateau, Plateau* derniere) {
    if (!derniere) return 0;
    int total_capture = 0;
    Plateau* courante = derniere;

    while (1) {
        int total = courante->R + courante->B + courante->T;
        if (total == 2 || total == 3) {
            total_capture += total;
            courante->R = courante->B = courante->T = 0;
            courante = case_precedente(plateau, courante);  // plus de nb_cases
        } else break;
    }
    return total_capture;
}


Plateau* distribuer(Plateau* depart, int couleur, int joueur, int mode_transp) {
    Plateau* courante = depart;
    Plateau* derniere = NULL;
    int graines = 0;

    // --- Cas normaux R ou B ---
    if (couleur == 1) { 
        graines = depart->R; 
        depart->R = 0; 
    }
    else if (couleur == 2) { 
        graines = depart->B; 
        depart->B = 0; 
    }
    else if (couleur == 3) { 
        graines = depart->T; 
        depart->T = 0; 
    }

    // --- Première phase : distribution classique ---
    while (graines > 0) {
        courante = courante->caseSuiv;
        if (courante == depart) continue; // ne pas redéposer dans la case d'origine

        if (couleur == 1) {
            courante->R++; graines--;
        } 
        else if (couleur == 2) {
            if (!case_du_joueur(courante->caseN, joueur)) { 
                courante->B++; graines--; 
            }
        } 
        else if (couleur == 3) {
            if (mode_transp == 1) { // joue comme rouge
                courante->T++; graines--;
            } 
            else if (mode_transp == 2) { // joue comme bleue
                if (!case_du_joueur(courante->caseN, joueur)) {
                    courante->T++; graines--;
                }
            } 
            else { // mode_transp == 0 → transparentes normales
                courante->T++; graines--;
            }
        }
        derniere = courante;
    }

    // --- Seconde phase : si T+R ou T+B ---
    if (couleur == 3) {
        // joue T+R
        if (mode_transp == 1) {
            int r = depart->R;
            depart->R = 0;
            while (r > 0) {
                courante = courante->caseSuiv;
                if (courante == depart) continue;
                courante->R++; 
                r--;
                derniere = courante;
            }
        }

        // joue T+B
        else if (mode_transp == 2) {
            int b = depart->B;
            depart->B = 0;
            while (b > 0) {
                courante = courante->caseSuiv;
                if (courante == depart) continue;
                if (!case_du_joueur(courante->caseN, joueur)) {
                    courante->B++; 
                    b--;
                    derniere = courante;
                }
            }
        }
    }

    return derniere;
}



// Joue un coup sur le plateau sans le copier
Plateau* jouer_coup(Plateau* plateau, Coup* c) {
    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        c->R[i] = p->R;
        c->B[i] = p->B;
        c->T[i] = p->T;
        p = p->caseSuiv;
    }

    Plateau* depart = trouver_case(plateau, c->case_depart);
    Plateau* derniere = distribuer(depart, c->couleur, c->joueur, c->mode_transp);
    c->derniere = derniere;
    c->captures = capturer(plateau, derniere);
    return derniere;
}

// Annule le dernier coup en restaurant l’état sauvegardé
void annuler_coup(Plateau* plateau, Coup* c) {
    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        p->R = c->R[i];
        p->B = c->B[i];
        p->T = c->T[i];
        p = p->caseSuiv;
    }
}

int verifier_conditions_fin(Partie* partie) {
    int totalJ1 = total_graines_joueur(partie->plateau, 1);
    int totalJ2 = total_graines_joueur(partie->plateau, 2);
    int total = totalJ1 + totalJ2;

    // Cas famine J1 → J2 récupère tout
    if (totalJ1 == 0 && totalJ2 > 0) {
        partie->scores[1] += totalJ2;
        return 1;
    }

    // Cas famine J2 → J1 récupère tout
    if (totalJ2 == 0 && totalJ1 > 0) {
        partie->scores[0] += totalJ1;
        return 1;
    }

    if (partie->scores[0] >= 49) {
        return 1;
    }
    if (partie->scores[1] >= 49) {
        return 1;
    }

    if (partie->scores[0] == 40 && partie->scores[1] == 40) {
        return 1; // match nul
    }

    // Trop peu de graines → égalité ou comparaison des scores
    if (total < 10) {
        return 1;
    }

    return 0;
}

// pour les fonctions d'évaluation, retourne :
// +1  si joueur gagne
// -1  si joueur perd
//  0  si partie non terminée
int evaluer_fin_de_partie(Plateau* plateau, int joueur) {

    int totalJ1 = total_graines_joueur(plateau, 1);
    int totalJ2 = total_graines_joueur(plateau, 2);
    int total = totalJ1 + totalJ2;

    // famine (joueur 1 affamé)
    if (totalJ1 == 0 && totalJ2 > 0) {
        return (joueur == 1) ? SCORE_DEFAITE : SCORE_VICTOIRE;
    }

    // famine (joueur 2 affamé)
    if (totalJ2 == 0 && totalJ1 > 0) {
        return (joueur == 2) ? SCORE_DEFAITE : SCORE_VICTOIRE;
    }

    // fin classique : peu de graines
    if (total < 10) {
        if (totalJ1 > totalJ2) 
            return (joueur == 1) ? SCORE_VICTOIRE : SCORE_DEFAITE;

        if (totalJ2 > totalJ1) 
            return (joueur == 2) ? SCORE_VICTOIRE : SCORE_DEFAITE;

        return 0; // égalité
    }

    return 0; // partie non terminée
}
