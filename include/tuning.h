#ifndef TUNING_H
#define TUNING_H

#include "plateau.h"
#include "jeu.h"

// Structure pour les paramètres d'évaluation (tunables)
struct ParametresEvaluation {
    // Coefficients pour evaluer_plateau_avance
    float poids_ressources_base;
    float poids_ressources_phase;
    float poids_mobilite_base;
    float poids_mobilite_phase;
    float poids_captures;
    float poids_transparentes;
    float bonus_menace_famine;
    float malus_cases_critiques;
    float bonus_cases_vides;
    
    // Coefficients pour evaluer_plateau_famine_avance
    float famine_vides;
    float famine_un;
    float famine_mobilite_base;
    float famine_mobilite_phase;
    float famine_ressources_base;
    float famine_ressources_phase;
    float famine_captures;
    float famine_menace;
    float famine_critiques;
};
typedef struct ParametresEvaluation ParametresEvaluation;

// Fonctions de tuning
void initialiser_parametres_defaut(ParametresEvaluation* params);
void tuner_parametres(ParametresEvaluation* params, int nb_parties);
float evaluer_parametres(ParametresEvaluation* params, int nb_parties_test);
void afficher_parametres(ParametresEvaluation* params);

// Fonctions d'évaluation avec paramètres
int evaluer_plateau_avance_tune(Plateau* plateau, int joueur, ParametresEvaluation* params);
int evaluer_plateau_famine_avance_tune(Plateau* plateau, int joueur, ParametresEvaluation* params);

#endif

