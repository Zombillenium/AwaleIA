#define _XOPEN_SOURCE 600   // pour déclarer usleep()
#include <unistd.h>         // pour usleep()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plateau.h"
#include "jeu.h"
#include "ia.h"
#include "tabletranspo.h"
#include "evaluation.h"
#include <MQTTClient.h>
#include <omp.h>
#include <stdlib.h> // pour getenv()

const char* ADDRESS;
const char* CLIENTID;
const char* USERNAME;
const char* PASSWORD;
const char* TOPIC_J0;
const char* TOPIC_J1;
#define TOPIC "awale/match"

#define QOS         1
#define TIMEOUT     10000L

MQTTClient client;

// === Publier un coup ===
void publier_coup(const char* topic, const char* coup) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (char*)coup;
    pubmsg.payloadlen = strlen(coup);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("📤 Coup publié (%s) : %s\n", topic, coup);
}


// === Callback : réception d’un message ===
int message_arrive(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
    (void)topicLen;  // éviter le warning "unused parameter"
    char* payload = (char*)message->payload;

    if (message->payloadlen >= 8) message->payloadlen = 7;
    payload[message->payloadlen] = '\0';

    printf("📥 Coup reçu : %s\n", payload);
    strncpy((char*)context, payload, 8);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main(int argc, char* argv[]) {
    // Charger les variables d'environnement
    ADDRESS   = getenv("MQTT_ADDRESS");
    CLIENTID  = getenv("MQTT_CLIENTID");
    USERNAME  = getenv("MQTT_USERNAME");
    PASSWORD  = getenv("MQTT_PASSWORD");
    TOPIC_J0  = getenv("MQTT_TOPIC_J0");
    TOPIC_J1  = getenv("MQTT_TOPIC_J1");

    if (!ADDRESS || !CLIENTID || !USERNAME || !PASSWORD || !TOPIC_J0 || !TOPIC_J1) {
        fprintf(stderr, "❌ Erreur : certaines variables d'environnement MQTT sont manquantes.\n");
        exit(EXIT_FAILURE);
    }
    if (argc < 2) {
        printf("Usage: %s <numero_joueur 0|1>\n", argv[0]);
        return 1;
    }

    int mon_joueur = atoi(argv[1]);
    printf("🤖 Lancement du joueur %d via MQTT\n", mon_joueur);

    // === Initialisation de l’IA et du plateau ===
    initialiser_zobrist();
    initialiser_transpo_lock();
    initialiser_killer_moves();

    Partie partie;
    partie.nb_cases = 16;
    partie.plateau = creer_plateau(partie.nb_cases);
    partie.scores[0] = partie.scores[1] = 0;
    partie.tour = 0;
    afficher_plateau(partie.plateau, partie.nb_cases);

    // === Configuration et connexion MQTT ===
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

    ssl_opts.enableServerCertAuth = 1;
    ssl_opts.verify = 1;
    ssl_opts.CApath = "/etc/ssl/certs";
    ssl_opts.trustStore = "/etc/ssl/certs/ca-certificates.crt";
    ssl_opts.sslVersion = MQTT_SSL_VERSION_TLS_1_2;

    conn_opts.keepAliveInterval = 60;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;
    conn_opts.ssl = &ssl_opts;
    

    char dernier_coup[8] = {0};
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, dernier_coup, NULL, message_arrive, NULL);

    printf("🔌 Tentative de connexion au serveur MQTT...\n");
    int rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "❌ Connexion MQTT échouée (code %d)\n", rc);
        fprintf(stderr, "⚠️ Vérifie ton mot de passe HiveMQ et la configuration SSL.\n");
        exit(EXIT_FAILURE);
    }

    MQTTClient_subscribe(client, TOPIC, QOS);
    printf("✅ Connecté à %s\n", ADDRESS);

    char topic_send[32], topic_recv[32];

    if (mon_joueur == 0) {
        strcpy(topic_send, TOPIC_J0);
        strcpy(topic_recv, TOPIC_J1);
    } else {
        strcpy(topic_send, TOPIC_J1);
        strcpy(topic_recv, TOPIC_J0);
    }

    MQTTClient_subscribe(client, topic_recv, QOS);
    printf("📡 Abonné au topic adversaire : %s\n", topic_recv);


    // === Boucle principale du jeu ===
    while (1) {
        if (partie.tour == mon_joueur) {
            printf("\n=== Mon tour (Joueur %d) ===\n", mon_joueur + 1);

            int best_case = -1, best_color = -1, best_mode = 0;
            choisir_meilleur_coup(&partie, mon_joueur + 1, &best_case, &best_color, &best_mode);

            char coup_str[8];
            if (best_color == 1) sprintf(coup_str, "%dR", best_case);
            else if (best_color == 2) sprintf(coup_str, "%dB", best_case);
            else if (best_color == 3 && best_mode == 1) sprintf(coup_str, "%dTR", best_case);
            else sprintf(coup_str, "%dTB", best_case);

            publier_coup(topic_send, coup_str);
            Plateau* c = trouver_case(partie.plateau, best_case);
            Plateau* derniere = distribuer(c, best_color, mon_joueur + 1, best_mode);
            int captures = capturer(partie.plateau, derniere, partie.nb_cases);
            partie.scores[mon_joueur] += captures;
            afficher_plateau(partie.plateau, partie.nb_cases);

            partie.tour = 1 - partie.tour;
        } else {
            printf("\n🕓 En attente du coup de l’adversaire...\n");
            while (strlen(dernier_coup) == 0) {
                MQTTClient_yield();
                usleep(100000);
            }

            printf("Coup adverse reçu : %s\n", dernier_coup);
            int c = 0, col = 0, mode = 0;
            sscanf(dernier_coup, "%d", &c);
            char* p = dernier_coup;
            while (*p && (*p >= '0' && *p <= '9')) p++;
            if (*p == 'R') col = 1;
            else if (*p == 'B') col = 2;
            else if (*p == 'T') { p++; col = 3; mode = (*p == 'R') ? 1 : 2; }

            Plateau* case_j = trouver_case(partie.plateau, c);
            Plateau* derniere = distribuer(case_j, col, 2 - mon_joueur, mode);
            int captures = capturer(partie.plateau, derniere, partie.nb_cases);
            partie.scores[1 - mon_joueur] += captures;
            afficher_plateau(partie.plateau, partie.nb_cases);

            strcpy(dernier_coup, "");
            partie.tour = 1 - partie.tour;
        }
    }

    // === Nettoyage ===
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    liberer_plateau(partie.plateau, partie.nb_cases);
    return 0;
}
