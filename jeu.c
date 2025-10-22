#include "jeu.h"
#include <stdlib.h>

int capturer(Plateau* plateau, Plateau* derniere) {
    int total_capture = 0;
    Plateau* courante = derniere;

    while (1) {
        int total = courante->R + courante->B + courante->T;
        if (total == 2 || total == 3) {
            total_capture += total;
            courante->R = courante->B = courante->T = 0;
            courante = case_precedente(plateau, courante);
        } else {
            break;
        }
    }
    return total_capture;
}

Plateau* distribuer(Plateau* depart, int couleur, int joueur, int mode_transp) {
    Plateau* courante = depart;
    int graines = 0;

    if (couleur == 1) { graines = depart->R; depart->R = 0; }
    else if (couleur == 2) { graines = depart->B; depart->B = 0; }
    else if (couleur == 3) { graines = depart->T; depart->T = 0; }

    Plateau* derniere = NULL;

    while (graines > 0) {
        courante = courante->caseSuiv;
        if (courante == depart) continue;

        if (couleur == 1) {
            courante->R++; graines--;
        } else if (couleur == 2) {
            if (!case_du_joueur(courante->caseN, joueur)) { courante->B++; graines--; }
        } else if (couleur == 3) {
            if (mode_transp == 1) { // joue comme rouge
                courante->T++; graines--;
            } else if (mode_transp == 2 && !case_du_joueur(courante->caseN, joueur)) { // comme bleue
                courante->T++; graines--;
            }
        }
        derniere = courante;
    }
    return derniere;
}
