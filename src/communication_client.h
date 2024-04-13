#ifndef SRC_COMMUNICATION_CLIENT_H_
#define SRC_COMMUNICATION_CLIENT_H_

#include "./messages.h"
#include "./model.h"

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq_id);

connection_information *recv_connexion_information(int sock);

#endif // SRC_COMMUNICATION_CLIENT_H_
