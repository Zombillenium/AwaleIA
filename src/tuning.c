#include "tuning.h"
#include "evaluation.h"
#include "ia.h"
#include "jeu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// =============================
// INITIALISATION PARAMÈTRES
// =============================

void initialiser_parametres_defaut(ParametresEvaluation* params) {
    // Valeurs par défaut (basées sur les valeurs actuelles)
    params->poids_ressources_base = 3.0f;
    params->poids_ressources_phase = 0.5f;
    params->poids_mobilite_base = 4.0f;
    params->poids_mobilite_phase = 1.0f;
    params->poids_captures = 6.0f;
    params->poids_transparentes = 1.5f;
    params->bonus_menace_famine = 50.0f;
    params->malus_cases_critiques = 15.0f;
    params->bonus_cases_vides = 8.0f;
    
    params->famine_vides = 40.0f;
    params->famine_un = 25.0f;
    params->famine_mobilite_base = 12.0f;
    params->famine_mobilite_phase = 1.2f;
    params->famine_ressources_base = 8.0f;
    params->famine_ressources_phase = 1.0f;
    params->famine_captures = 12.0f;
    params->famine_menace = 60.0f;
    params->famine_critiques = 20.0f;
}

// =============================
// ÉVALUATION AVEC PARAMÈTRES
// =============================

int evaluer_plateau_avance_tune(Plateau* plateau, int joueur, ParametresEvaluation* params) {
    int totalJ = 0, totalA = 0;
    int transJ = 0, transA = 0;
    int casesCapturablesJ = 0, casesCapturablesA = 0;
    int casesVidesJ = 0, casesVidesA = 0;
    int coupsLegauxJ = 0, coupsLegauxA = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            totalJ += total;
            transJ += p->T;
            if (total == 2 || total == 3) casesCapturablesJ++;
            if (total == 0) casesVidesJ++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxJ++;
        } else {
            totalA += total;
            transA += p->T;
            if (total == 2 || total == 3) casesCapturablesA++;
            if (total == 0) casesVidesA++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    float phase = calculer_phase_jeu(plateau);
    int menace_famine = detecter_menace_famine(plateau, joueur);
    int cases_critiques_j = compter_cases_critiques(plateau, joueur);
    int cases_critiques_a = compter_cases_critiques(plateau, 3 - joueur);
    
    float score = 0.0f;
    
    float poids_ressources = params->poids_ressources_base + phase * params->poids_ressources_phase;
    score += poids_ressources * totalJ;
    score -= poids_ressources * 0.67f * totalA; // Ratio 2/3
    
    float poids_mobilite = params->poids_mobilite_base - phase * params->poids_mobilite_phase;
    score += poids_mobilite * (coupsLegauxJ - coupsLegauxA);
    
    score += params->poids_captures * (casesCapturablesJ - casesCapturablesA);
    score += params->poids_transparentes * (transJ - transA);
    
    if (menace_famine) score += params->bonus_menace_famine;
    score -= params->malus_cases_critiques * cases_critiques_j;
    score += params->malus_cases_critiques * cases_critiques_a;
    score += params->bonus_cases_vides * (casesVidesA - casesVidesJ);
    
    if (score < -200.0f) score = -200.0f;
    if (score > 200.0f) score = 200.0f;
    
    int eval = (int)(500.0f + (score * 2.5f));
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;
    
    return eval;
}

int evaluer_plateau_famine_avance_tune(Plateau* plateau, int joueur, ParametresEvaluation* params) {
    if (!plateau) return 0;

    int res = evaluer_fin_de_partie(plateau, joueur);
    if (res == SCORE_VICTOIRE) return SCORE_VICTOIRE;
    if (res == SCORE_DEFAITE) return SCORE_DEFAITE;

    int grainesJ = 0, grainesA = 0;
    int casesVidesJ = 0, casesVidesA = 0;
    int coupsJ = 0, coupsA = 0;
    int captJ = 0, captA = 0;
    int famine1J = 0, famine1A = 0;
    int coupsLegauxJ = 0, coupsLegauxA = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            grainesJ += total;
            if (total == 0) casesVidesJ++;
            if (total > 0) coupsJ++;
            if (total == 2 || total == 3) captJ++;
            if (total == 1) famine1J++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxJ++;
        } else {
            grainesA += total;
            if (total == 0) casesVidesA++;
            if (total > 0) coupsA++;
            if (total == 2 || total == 3) captA++;
            if (total == 1) famine1A++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    float phase = calculer_phase_jeu(plateau);
    int menace_famine = detecter_menace_famine(plateau, joueur);
    int cases_critiques_j = compter_cases_critiques(plateau, joueur);
    int cases_critiques_a = compter_cases_critiques(plateau, 3 - joueur);
    
    float score = 0.0f;

    score += params->famine_vides * (casesVidesA - casesVidesJ);
    score += params->famine_un * (famine1A - famine1J);

    float poids_mobilite = params->famine_mobilite_base - phase * params->famine_mobilite_phase;
    float poids_ressources = params->famine_ressources_base + phase * params->famine_ressources_phase;
    
    score += poids_mobilite * (coupsLegauxJ - coupsLegauxA);
    score += poids_ressources * (grainesJ - grainesA);
    score += params->famine_captures * (captJ - captA);
    score += phase * 25.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    if (menace_famine) score += params->famine_menace;
    score -= params->famine_critiques * cases_critiques_j;
    score += params->famine_critiques * cases_critiques_a;
    
    if (grainesA <= 6) score += (35 - 3 * grainesA);
    if (coupsLegauxA <= 2) score += 30;
    if (casesVidesA >= 6) score += 25;
    if (grainesJ <= 6) score -= (35 - 3 * grainesJ);
    if (coupsLegauxJ <= 2) score -= 30;
    if (casesVidesJ >= 6) score -= 25;

    if (grainesA < grainesJ - 6) {
        score += 18.0f * (captJ - captA);
        score += 10.0f * (coupsLegauxJ - coupsLegauxA);
    }

    if (grainesJ > grainesA + 8) {
        score += 0.6f * (grainesJ - grainesA);
    }

    return (int)score;
}

// =============================
// TUNING AUTOMATIQUE
// =============================

// Version rapide de choisir_meilleur_coup pour le tuning (profondeur limitée, silencieuse, avec paramètres)
static int choisir_meilleur_coup_rapide(Partie* partie, int joueur, int* best_case, int* best_color, int* best_mode, 
                                         int profondeur_max, ParametresEvaluation* params) {
    Plateau* plateau = partie->plateau;
    
    // Version simplifiée : profondeur fixe, pas de recherche itérative
    Move moves[nb_cases * 3 * 2];
    int nmoves = 0;
    Plateau* p = plateau;
    
    for (int i = 0; i < nb_cases; i++) {
        if (!case_du_joueur(p->caseN, joueur)) {
            p = p->caseSuiv;
            continue;
        }
        if (p->R > 0) moves[nmoves++] = (Move){p->caseN, 1, 0};
        if (p->B > 0) moves[nmoves++] = (Move){p->caseN, 2, 0};
        if (p->T > 0) {
            if (p->T + p->R > 0) moves[nmoves++] = (Move){p->caseN, 3, 1};
            if (p->T + p->B > 0) moves[nmoves++] = (Move){p->caseN, 3, 2};
        }
        p = p->caseSuiv;
    }
    
    if (nmoves == 0) return 0;
    
    // Recherche simplifiée avec profondeur limitée et paramètres tunés
    int meilleur_score = -999999;
    int best_idx = -1;
    
    for (int k = 0; k < nmoves; k++) {
        Move mv = moves[k];
        Plateau* copie = copier_plateau(plateau);
        Plateau* case_copie = trouver_case(copie, mv.caseN);
        Plateau* derniere = distribuer(case_copie, mv.couleur, joueur, (mv.couleur == 3 ? mv.mode : 0));
        int captures = capturer(copie, derniere);
        
        // Recherche minimax limitée avec paramètres tunés
        int alpha = -100000;
        int beta = 100000;
        int score = minimax_tune(copie, joueur, profondeur_max - 1, alpha, beta, 1, params) + captures * 5;
        
        if (score > meilleur_score) {
            meilleur_score = score;
            best_idx = k;
        }
        liberer_plateau(copie);
    }
    
    if (best_idx >= 0) {
        *best_case = moves[best_idx].caseN;
        *best_color = moves[best_idx].couleur;
        *best_mode = (moves[best_idx].couleur == 3 ? moves[best_idx].mode : 0);
        return 1;
    }
    
    return 0;
}

// Simule une partie complète avec les paramètres donnés
int simuler_partie(ParametresEvaluation* params_j1, ParametresEvaluation* params_j2) {
    Partie partie;
    partie.plateau = creer_plateau();
    partie.scores[0] = partie.scores[1] = 0;
    partie.tour = 0;
    
    int tours = 0;
    int profondeur_max = 5; // Profondeur 5 pour le tuning (bon compromis vitesse/précision)
    
    while (tours < 200) { // Limite de sécurité
        int joueur = partie.tour + 1;
        
        int best_case = -1, best_color = -1, best_mode = 0;
        // Utiliser la version rapide avec profondeur limitée et paramètres tunés
        ParametresEvaluation* params = (joueur == 1) ? params_j1 : params_j2;
        int ok = choisir_meilleur_coup_rapide(&partie, joueur, &best_case, &best_color, &best_mode, profondeur_max, params);
        
        if (!ok || verifier_conditions_fin(&partie)) break;
        
        Plateau* c = trouver_case(partie.plateau, best_case);
        Plateau* derniere = distribuer(c, best_color, joueur, best_mode);
        int captures = capturer(partie.plateau, derniere);
        partie.scores[joueur - 1] += captures;
        
        partie.tour = 1 - partie.tour;
        tours++;
    }
    
    liberer_plateau(partie.plateau);
    
    // Retourne le gagnant (1 ou 2) ou 0 pour match nul
    if (partie.scores[0] > partie.scores[1]) return 1;
    if (partie.scores[1] > partie.scores[0]) return 2;
    return 0;
}

// Fonction utilitaire pour afficher une barre de progression
static void afficher_barre_progression(int actuel, int total, double temps_ecoule, double temps_estime) {
    const int largeur_barre = 40;
    int rempli = (int)((actuel / (float)total) * largeur_barre);
    
    printf("\r[");
    for (int i = 0; i < largeur_barre; i++) {
        if (i < rempli) printf("=");
        else if (i == rempli) printf(">");
        else printf(" ");
    }
    printf("] %d/%d (%.1f%%) | Temps: %.1fs | Restant: %.1fs",
           actuel, total, (actuel * 100.0f / total), temps_ecoule, temps_estime);
    fflush(stdout);
}

// Fonction pour formater le temps en heures:minutes:secondes
static void formater_temps(double secondes, char* buffer, int taille) {
    int h = (int)(secondes / 3600);
    int m = (int)((secondes - h * 3600) / 60);
    int s = (int)(secondes - h * 3600 - m * 60);
    
    if (h > 0) {
        snprintf(buffer, taille, "%dh %dm %ds", h, m, s);
    } else if (m > 0) {
        snprintf(buffer, taille, "%dm %ds", m, s);
    } else {
        snprintf(buffer, taille, "%ds", s);
    }
}

// Évalue les paramètres en jouant plusieurs parties
float evaluer_parametres(ParametresEvaluation* params, int nb_parties_test) {
    ParametresEvaluation params_defaut;
    initialiser_parametres_defaut(&params_defaut);
    
    int victoires = 0;
    int matchs_nuls = 0;
    int defaites = 0;
    
    struct timeval debut, maintenant;
    gettimeofday(&debut, NULL);
    
    // Vérification que les paramètres sont différents
    int params_differents = (params->poids_captures != params_defaut.poids_captures ||
                            params->bonus_menace_famine != params_defaut.bonus_menace_famine ||
                            params->famine_vides != params_defaut.famine_vides);
    
    for (int i = 0; i < nb_parties_test; i++) {
        // Joue avec les nouveaux paramètres (J1) contre les défauts (J2)
        int resultat = simuler_partie(params, &params_defaut);
        if (resultat == 1) victoires++;
        else if (resultat == 2) defaites++;
        else if (resultat == 0) matchs_nuls++;
        
        // Joue aussi l'inverse : défauts (J1) vs nouveaux paramètres (J2)
        resultat = simuler_partie(&params_defaut, params);
        if (resultat == 2) victoires++; // J2 (nouveaux paramètres) gagne
        else if (resultat == 1) defaites++; // J1 (défauts) gagne
        else if (resultat == 0) matchs_nuls++;
        
        // Affichage de la progression
        gettimeofday(&maintenant, NULL);
        double temps_ecoule = (maintenant.tv_sec - debut.tv_sec) + 
                             (maintenant.tv_usec - debut.tv_usec) / 1000000.0;
        double temps_par_partie = temps_ecoule / (i + 1);
        double temps_restant = temps_par_partie * (nb_parties_test * 2 - (i + 1) * 2);
        
        afficher_barre_progression(i + 1, nb_parties_test, temps_ecoule, temps_restant);
    }
    
    printf("\n"); // Nouvelle ligne après la barre
    
    // Score = % de victoires + 0.5 * % de matchs nuls
    // Plus informatif : on affiche aussi les stats
    int total_parties = nb_parties_test * 2;
    float score = (victoires / (float)total_parties) * 100.0f + 
                  (matchs_nuls / (float)total_parties) * 50.0f;
    
    // Debug : afficher les stats si les paramètres sont différents
    if (params_differents && nb_parties_test >= 3) {
        printf("   Stats: %d victoires, %d défaites, %d nuls (sur %d parties)\n", 
               victoires, defaites, matchs_nuls, total_parties);
    }
    
    return score;
}

// Affiche proprement les paramètres
void afficher_parametres(ParametresEvaluation* params) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              PARAMÈTRES D'ÉVALUATION                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║ Évaluation classique (evaluer_plateau_avance)            ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Ressources: base=%.2f, phase=%.2f                        ║\n", 
           params->poids_ressources_base, params->poids_ressources_phase);
    printf("║  Mobilité:   base=%.2f, phase=%.2f                        ║\n", 
           params->poids_mobilite_base, params->poids_mobilite_phase);
    printf("║  Captures:  %.2f                                         ║\n", params->poids_captures);
    printf("║  Transparentes: %.2f                                     ║\n", params->poids_transparentes);
    printf("║  Bonus menace famine: %.2f                              ║\n", params->bonus_menace_famine);
    printf("║  Malus cases critiques: %.2f                            ║\n", params->malus_cases_critiques);
    printf("║  Bonus cases vides: %.2f                                 ║\n", params->bonus_cases_vides);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║ Évaluation famine (evaluer_plateau_famine_avance)       ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Famine vides: %.2f                                      ║\n", params->famine_vides);
    printf("║  Famine un: %.2f                                         ║\n", params->famine_un);
    printf("║  Mobilité: base=%.2f, phase=%.2f                        ║\n", 
           params->famine_mobilite_base, params->famine_mobilite_phase);
    printf("║  Ressources: base=%.2f, phase=%.2f                       ║\n", 
           params->famine_ressources_base, params->famine_ressources_phase);
    printf("║  Captures: %.2f                                         ║\n", params->famine_captures);
    printf("║  Bonus menace: %.2f                                     ║\n", params->famine_menace);
    printf("║  Malus critiques: %.2f                                   ║\n", params->famine_critiques);
    printf("╚═══════════════════════════════════════════════════════════╝\n");
}

// Tuning par grid search simplifié (variation autour des valeurs par défaut)
void tuner_parametres(ParametresEvaluation* params, int nb_parties) {
    printf("\n🔧 Début du tuning automatique (%d parties par test)...\n\n", nb_parties);
    
    struct timeval debut_total, maintenant;
    gettimeofday(&debut_total, NULL);
    
    ParametresEvaluation meilleurs = *params;
    
    // Test initial
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📊 Test initial (baseline)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    float meilleur_score = evaluer_parametres(&meilleurs, nb_parties);
    printf("📊 Score initial : %.2f%%\n\n", meilleur_score);
    
    // Grid search sur les paramètres les plus importants
    float variations[] = {0.8f, 0.9f, 1.0f, 1.1f, 1.2f};
    int nb_variations = 5;
    int total_tests = 1 + (nb_variations * 3); // initial + 3 paramètres × 5 variations
    int test_actuel = 1;
    
    // Tune poids_captures (très important)
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔍 Tuning: poids_captures (test %d/%d)\n", test_actuel, total_tests);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    for (int i = 0; i < nb_variations; i++) {
        ParametresEvaluation test = meilleurs;
        test.poids_captures = params->poids_captures * variations[i];
        
        gettimeofday(&maintenant, NULL);
        double temps_ecoule = (maintenant.tv_sec - debut_total.tv_sec) + 
                             (maintenant.tv_usec - debut_total.tv_usec) / 1000000.0;
        double temps_par_test = temps_ecoule / test_actuel;
        double temps_restant = temps_par_test * (total_tests - test_actuel);
        
        char temps_str[64], reste_str[64];
        formater_temps(temps_ecoule, temps_str, sizeof(temps_str));
        formater_temps(temps_restant, reste_str, sizeof(reste_str));
        
        printf("\n🔹 Test variation %.1fx (valeur: %.2f) | Temps: %s | Restant: %s\n", 
               variations[i], test.poids_captures, temps_str, reste_str);
        
        float score = evaluer_parametres(&test, nb_parties / 2);
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleurs = test;
            printf("✅ ⭐ AMÉLIORATION ⭐ poids_captures = %.2f (score: %.2f%%)\n", 
                   test.poids_captures, score);
        } else {
            printf("   Score: %.2f%% (pas d'amélioration)\n", score);
        }
        test_actuel++;
    }
    
    // Tune bonus_menace_famine
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔍 Tuning: bonus_menace_famine (test %d/%d)\n", test_actuel, total_tests);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    for (int i = 0; i < nb_variations; i++) {
        ParametresEvaluation test = meilleurs;
        test.bonus_menace_famine = params->bonus_menace_famine * variations[i];
        
        gettimeofday(&maintenant, NULL);
        double temps_ecoule = (maintenant.tv_sec - debut_total.tv_sec) + 
                             (maintenant.tv_usec - debut_total.tv_usec) / 1000000.0;
        double temps_par_test = temps_ecoule / test_actuel;
        double temps_restant = temps_par_test * (total_tests - test_actuel);
        
        char temps_str[64], reste_str[64];
        formater_temps(temps_ecoule, temps_str, sizeof(temps_str));
        formater_temps(temps_restant, reste_str, sizeof(reste_str));
        
        printf("\n🔹 Test variation %.1fx (valeur: %.2f) | Temps: %s | Restant: %s\n", 
               variations[i], test.bonus_menace_famine, temps_str, reste_str);
        
        float score = evaluer_parametres(&test, nb_parties / 2);
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleurs = test;
            printf("✅ ⭐ AMÉLIORATION ⭐ bonus_menace_famine = %.2f (score: %.2f%%)\n", 
                   test.bonus_menace_famine, score);
        } else {
            printf("   Score: %.2f%% (pas d'amélioration)\n", score);
        }
        test_actuel++;
    }
    
    // Tune famine_vides
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔍 Tuning: famine_vides (test %d/%d)\n", test_actuel, total_tests);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    for (int i = 0; i < nb_variations; i++) {
        ParametresEvaluation test = meilleurs;
        test.famine_vides = params->famine_vides * variations[i];
        
        gettimeofday(&maintenant, NULL);
        double temps_ecoule = (maintenant.tv_sec - debut_total.tv_sec) + 
                             (maintenant.tv_usec - debut_total.tv_usec) / 1000000.0;
        double temps_par_test = temps_ecoule / test_actuel;
        double temps_restant = temps_par_test * (total_tests - test_actuel);
        
        char temps_str[64], reste_str[64];
        formater_temps(temps_ecoule, temps_str, sizeof(temps_str));
        formater_temps(temps_restant, reste_str, sizeof(reste_str));
        
        printf("\n🔹 Test variation %.1fx (valeur: %.2f) | Temps: %s | Restant: %s\n", 
               variations[i], test.famine_vides, temps_str, reste_str);
        
        float score = evaluer_parametres(&test, nb_parties / 2);
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleurs = test;
            printf("✅ ⭐ AMÉLIORATION ⭐ famine_vides = %.2f (score: %.2f%%)\n", 
                   test.famine_vides, score);
        } else {
            printf("   Score: %.2f%% (pas d'amélioration)\n", score);
        }
        test_actuel++;
    }
    
    *params = meilleurs;
    
    gettimeofday(&maintenant, NULL);
    double temps_total = (maintenant.tv_sec - debut_total.tv_sec) + 
                         (maintenant.tv_usec - debut_total.tv_usec) / 1000000.0;
    char temps_total_str[64];
    formater_temps(temps_total, temps_total_str, sizeof(temps_total_str));
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🎯 Tuning terminé en %s\n", temps_total_str);
    printf("📊 Score final : %.2f%%\n", meilleur_score);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
}

