#include <arpa/inet.h>
#include <stdint.h>

#ifndef SRC_COMMUNICATION_SERVER_H_
#define SRC_COMMUNICATION_SERVER_H_

#include "./messages.h"
#include "./model.h"

int send_connexion_information(int sock, GAME_MODE mode, int id, int eq, int port_udp, int portmdiff,
                               uint16_t adrmdiff[8]);
int send_game_board(int sock, struct sockaddr_in6 *addr_mult, uint16_t num, board *board_);
int send_game_update(int sock, struct sockaddr_in6 *addr_mult, int num, tile_diff *diff, uint8_t nb);

initial_connection_header *recv_initial_connection_header(int sock);
ready_connection_header *recv_ready_connexion_header(int sock);
game_action *recv_game_action(int sock);

#endif // SRC_COMMUNICATION_SERVER_H_
