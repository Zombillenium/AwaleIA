#ifndef JEU_H
#define JEU_H

#include "plateau.h"

// couleur: 1=R, 2=B, 3=T ; mode_transp: 1 = comme R, 2 = comme B (ignoré sinon)
Plateau* distribuer(Plateau* depart, int couleur, int joueur, int mode_transp);

// capture à partir de la dernière case semée, en remontant tant que total ∈ {2,3}
int capturer(Plateau* plateau, Plateau* derniere);

#endif
