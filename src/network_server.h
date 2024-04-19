#ifndef SRC_NETWORK_SERVER_H_
#define SRC_NETWORK_SERVER_H_

#include "communication_server.h"

#define MIN_PORT 1024
#define MAX_PORT 49151

void close_socket_tcp();
void close_socket_udp();
void close_socket_mult();
void close_socket_client(int id);
int init_socket_tcp();
int init_socket_udp();
int init_socket_mult();
uint16_t get_port_tcp();
int try_to_init_socket_of_client(int id);
int try_to_bind_random_port_on_socket_tcp();
int try_to_bind_random_port_on_socket_udp();
int init_random_port_on_socket_mult();
void free_addr_mult();
int init_random_adrmdiff();
int init_addr_mult();
int listen_players();
initial_connection_header *recv_initial_connection_header_of_client(int id);
ready_connection_header *recv_ready_connexion_header_of_client(int id);
int send_connexion_information_of_client(int id, int eq);

#endif // SRC_NETWORK_SERVER_H__H_
