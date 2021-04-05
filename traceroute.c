#include "loggers.h"
#include "traceroute.h"

#include <stdio.h>
#include <nl_types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

/* Creates raw socket in domain IPv4, ICMP protocol */
int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd == -1)
    {
        log_error("creating socket");
        return EXIT_FAILURE;
    }
    return sockfd;
}

/*
* Receiving packets
*/

/* Waits for packet to be received */
int wait_for_packet(int sockfd, struct timeval *tv)
{
    fd_set descriptors;           // macros to manipulate set of sockets
    FD_ZERO(&descriptors);        // clears set of sockets
    FD_SET(sockfd, &descriptors); // adds descriptor to set

    // monitor multiple socket descriptors
    // (until socket is ready for being read)
    int ready = select(
        sockfd + 1,   // highest numbered descriptor plus 1
        &descriptors, // waited socket descriptors for read
        NULL,         // waited for write
        NULL,         // waited for exceptions
        tv);          // timeout (defined above)

    if (ready < 0)
    {
        log_error("waiting for packet");
        return EXIT_FAILURE;
    }
    else if (ready == 0)
    {
        return TIMEOUT;
    }

    return 1;
}

/* Converts the address structure in sender to a string with an IP address */
char sender_address_to_text(struct sockaddr_in *sender, char *sender_ip_text)
{
    const char *address_text_form = inet_ntop(
        AF_INET,             // IPv4 network address
        &(sender->sin_addr), // network address structure
        sender_ip_text,      // buffer
        20);                 // buffer length

    if (address_text_form == NULL)
    {
        log_error("converting address to text form");
        return EXIT_FAILURE;
    }

    printf("%s\n", sender_ip_text);
    return *sender_ip_text;
}

/* Reads IP header */
ssize_t read_ip_header(u_int8_t *buffer)
{
    struct ip *ip_header = (struct ip *)buffer;
    ssize_t ip_header_len = 4 * ip_header->ip_hl;

    return ip_header_len;
}

/* Reads ICMP header */
struct icmphdr *read_icmp_header(u_int8_t *buffer, ssize_t ip_header_len)
{
    return (struct icmphdr *)(buffer + ip_header_len);
}

/* Receives one data packet from socket */
int receive_one_packet(int sockfd, int ttl)
{
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    ssize_t packet_len = recvfrom(
        sockfd,                     // socket descriptor
        buffer,                     // saves read bytes as bytes sequence in buffer
        IP_MAXPACKET,               // read that many bytes
        0,                          // no flags
        (struct sockaddr *)&sender, // sender data
        &sender_len                 // sender data length
    );

    if (packet_len == -1)
    {
        log_error("receiving packet");
        return EXIT_FAILURE;
    }

    char sender_ip_text[20];
    if (sender_address_to_text(&sender, sender_ip_text) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    ssize_t ip_header_len = read_ip_header(buffer);
    struct icmphdr *icmp_header = read_icmp_header(buffer, ip_header_len);
    int id = icmp_header->un.echo.id;
    int seq_number = icmp_header->un.echo.sequence;

    // log_receive_one_packet_debug_msg(buffer, ip_header_len, packet_len);

    if (icmp_header->type == ICMP_ECHOREPLY && WAS_RECENT(ttl, seq_number) && id == getpid())
    {
        return ICMP_ECHOREPLY;
    }

    return EXIT_FAILURE;
}

/* Updates current response time */
void increase_current_time(struct timeval *tv, struct timeval *current_time)
{
    tv->tv_sec = MAX_WAITING_SECONDS;
    tv->tv_usec = 0;
    timersub(tv, current_time, tv);
}

/* Waits and receives packets */
int receive_packets(int sockfd, int ttl)
{
    int receive_one_packet_status;
    int received = 0;
    int has_receiver_responded = 0;

    struct timeval tv;
    struct timeval current_time;
    current_time.tv_sec = MAX_WAITING_SECONDS; // seconds
    current_time.tv_usec = 0;                  // microseconds

    printf("%i. ", ttl);

    while (received < ECHO_REQUESTS)
    {
        int wait_for_packet_status = wait_for_packet(sockfd, &current_time);
        if (wait_for_packet_status == EXIT_FAILURE ||
            wait_for_packet_status == TIMEOUT)
        {
            break;
        }

        receive_one_packet_status = receive_one_packet(sockfd, ttl);

        if (receive_one_packet_status == ICMP_ECHOREPLY)
        {
            has_receiver_responded = 1;
        }
        else if (receive_one_packet_status == EXIT_FAILURE)
        {
            continue;
        }

        increase_current_time(&tv, &current_time);
        received++;
    }

    // log_receivers()

    log_router_addresses(received, &current_time);

    return has_receiver_responded;
}

/*
* Sending packets
*/

/* Computes ICMP checksum */
u_int16_t compute_icmp_checksum(const void *buff, int length)
{
    u_int32_t sum;
    const u_int16_t *ptr = buff;
    assert(length % 2 == 0);

    for (sum = 0; length > 0; length -= 2)
        sum += *ptr++;

    sum = (sum >> 16) + (sum & 0xffff);

    return (u_int16_t)(~(sum + (sum >> 16)));
}

/* Initializes ICMP message with all necessary data */
struct icmp init_icmp_data(int seq_number)
{
    struct icmp header;
    header.icmp_type = ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = getpid();    // unique ID
    header.icmp_hun.ih_idseq.icd_seq = seq_number; // sequence number
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum(
        (uint16_t *)&header,
        sizeof(header));

    return header;
}

/* Sends ICMP packet */
ssize_t send_packet(int sockfd, struct icmp *header, struct sockaddr_in *recipient)
{
    ssize_t bytes_sent = sendto(
        sockfd,                       // socket descriptor
        header,                       // buffer
        sizeof(struct icmp),          // buffer size
        0,                            // no flags
        (struct sockaddr *)recipient, // ICMP packet recipient
        sizeof(struct sockaddr_in));  // recipient size

    if (bytes_sent == -1)
    {
        log_error("sending packet");
        return EXIT_FAILURE;
    }

    return bytes_sent;
}

/* Sets all necessary socket options */
int set_socket_options(int sockfd, int current_ttl)
{
    int set_socket_options_status = setsockopt(
        sockfd,       // socket descriptor to be set
        IPPROTO_IP,   // protocol level
        IP_TTL,       // option to set (IP_TTL = IP time to live)
        &current_ttl, // current time to live
        sizeof(int)); // option size

    if (set_socket_options_status == -1)
    {
        log_error("setting socket options");
        return EXIT_FAILURE;
    }

    return 1;
}

/*
* Traceroute
*/

/* Runs traceroute */
void traceroute(int sockfd, struct sockaddr_in *recipient)
{
    struct icmp header;
    int seq_number;
    int set_socket_options_status;
    int send_packet_status;
    int receive_packets_status;

    for (int i = 1; i <= TTL; ++i)
    {
        for (int j = 0; j < ECHO_REQUESTS; ++j)
        {
            seq_number = SEQ_NUMBER(i, j);
            header = init_icmp_data(seq_number);

            set_socket_options_status = set_socket_options(sockfd, i);
            if (set_socket_options_status == EXIT_FAILURE)
            {
                break;
            }

            send_packet_status = send_packet(sockfd, &header, recipient);
            if (send_packet_status == EXIT_FAILURE)
            {
                break;
            }
        }

        receive_packets_status = receive_packets(sockfd, i);
        if (receive_packets_status)
        {
            return;
        }
    }
}