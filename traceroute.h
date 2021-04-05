#include <arpa/inet.h>

#define EXIT_FAILURE -1
#define TIMEOUT 0
#define MAX_WAITING_SECONDS 1

#define TTL 30
#define ECHO_REQUESTS 3

#define SEQ_NUMBER(ttl, index) (((ttl) << 2) | (index))
#define WAS_RECENT(ttl, seq_number) ((ttl) == ((seq_number) >> 2))

int create_socket();

int wait_for_packet(int sockfd, struct timeval *tv);

int sender_address_to_text(struct sockaddr_in *sender, char *sender_ip_text);

ssize_t read_ip_header(u_int8_t *buffer);

struct icmphdr *read_icmp_header(u_int8_t *buffer, ssize_t ip_header_len);

int receive_one_packet(int sockfd, int ttl, char *receiver_ip_text);

void increase_current_time(struct timeval *tv, struct timeval *current_time);

int receive_packets(int sockfd, int ttl);

u_int16_t compute_icmp_checksum(const void *buff, int length);

struct icmp init_icmp_data(int seq_number);

ssize_t send_packet(int sockfd, struct icmp *header, struct sockaddr_in *recipient);

int set_socket_options(int sockfd, int current_ttl);

void traceroute(int sockfd, struct sockaddr_in *recipient);