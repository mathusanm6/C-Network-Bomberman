#ifndef SRC_COMMUNICATION_SERVER_H_
#define SRC_COMMUNICATION_SERVER_H_

#include "./messages.h"
#include "./model.h"

int send_connexion_information(int sock, GAME_MODE mode, int id, int eq, int port_udp, int portmdiff,
                               uint16_t adrmdiff[8]);

initial_connection_header *recv_initial_connection_header(int sock);
ready_connection_header *recv_ready_connexion_header(int sock);

#endif // SRC_COMMUNICATION_SERVER_H_
