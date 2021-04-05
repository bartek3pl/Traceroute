#include <arpa/inet.h>

void log_error(char *msg);

void print_as_bytes(unsigned char *buff, ssize_t length);

void log_receive_one_packet_debug_msg(u_int8_t *buffer,
                                      ssize_t ip_header_len,
                                      ssize_t packet_len);

void log_average_time(int received, struct timeval *current_time);

void log_router_addresses(int received, struct timeval *current_time);

void log_senders(int received, char *senders_ip_text[3]);