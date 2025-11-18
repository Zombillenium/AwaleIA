#include <stdio.h>
#include <stdlib.h>
#include "tuning.h"
#include "tabletranspo.h"
#include "ia.h"

int main(int argc, char* argv[]) {
    int nb_parties = 10;
    
    if (argc > 1) {
        nb_parties = atoi(argv[1]);
    }
    
    // Minimum 5 parties pour avoir une signification statistique minimale
    if (nb_parties < 5) {
        printf("⚠️  Nombre de parties trop faible (%d). Minimum recommandé : 5\n", nb_parties);
        printf("   Utilisation de 5 parties minimum.\n\n");
        nb_parties = 5;
    }
    
    printf("🔧 Système de tuning automatique des paramètres d'évaluation\n");
    printf("📊 Nombre de parties par test : %d\n", nb_parties);
    printf("💡 Note : Plus de parties = meilleure précision mais plus lent\n\n");
    
    initialiser_zobrist();
    initialiser_transpo_lock();
    initialiser_killer_moves();
    
    ParametresEvaluation params;
    initialiser_parametres_defaut(&params);
    
    printf("📋 Paramètres initiaux :\n");
    afficher_parametres(&params);
    
    // Test du score initial
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📊 Évaluation du score initial...\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    float score_initial = evaluer_parametres(&params, nb_parties);
    printf("\n📊 Score initial : %.2f%%\n\n", score_initial);
    
    // Tuning
    tuner_parametres(&params, nb_parties);
    
    printf("\n📋 Paramètres finaux optimisés :\n");
    afficher_parametres(&params);
    
    printf("💡 Note : Pour utiliser ces paramètres, il faut modifier les fonctions\n");
    printf("   d'évaluation dans evaluation.c pour utiliser les valeurs tunées.\n");
    printf("   Ou intégrer le système de paramètres dans minimax.\n");
    
    return 0;
}

