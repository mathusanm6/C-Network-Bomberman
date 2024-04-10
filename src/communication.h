#ifndef SRC_COMMUNICATION_H_
#define SRC_COMMUNICATION_H_

#include "./messages.h"
#include "./model.h"

#include <stdint.h>

int send_initial_connexion_information(int sock, GAME_MODE mode);
int send_ready_connexion_information(int sock, GAME_MODE mode, int id, int eq_id);
int send_connexion_information(int sock, GAME_MODE mode, int id, int eq_id, int port_udp, int portmdiff,
                               uint16_t adrmdiff[8]);

initial_connection_header *recv_initial_connection_header(int sock);
ready_connection_header *recv_ready_connexion_header(int sock);
connection_information *recv_connexion_information(int sock);

#endif // SRC_COMMUNICATION_H_
