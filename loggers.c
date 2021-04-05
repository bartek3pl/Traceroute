#include "loggers.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

/* Logs error messages */
void log_error(char *msg)
{
    printf("Something went wrong during %s: %s\n", msg, strerror(errno));
}

/* Prints buffer as bytes */
void print_as_bytes(unsigned char *buff, ssize_t length)
{
    for (ssize_t i = 0; i < length; i++, buff++)
    {
        printf("%.2x ", *buff);
    }
    printf("\n");
}

/* Logs debug message for receive_one_packet function */
void log_receive_one_packet_debug_msg(u_int8_t *buffer, ssize_t ip_header_len,
                                      ssize_t packet_len)
{
    printf("IP header: ");
    print_as_bytes(buffer, ip_header_len);

    printf("IP data: ");
    print_as_bytes(buffer + ip_header_len, packet_len - ip_header_len);

    printf("\n\n");
}

/* Logs average time of requests */
void log_average_time(int received, struct timeval *current_time)
{
    int tv_usec_sum = 0;
    for (int i = 0; i < received; ++i)
    {
        tv_usec_sum += current_time->tv_usec;
    }

    int average = tv_usec_sum / 3000;
    printf("%i ms\n", average);
}

/* Logs trace router address or logs average time */
void log_router_addresses(int received, struct timeval *current_time)
{
    if (!received)
    {
        printf("*\n");
    }
    else if (received < 3)
    {
        printf("???\n");
    }
    else
    {
        log_average_time(received, current_time);
    }
}

/* Logs all senders of packet sent */
void log_senders(int received, char *senders_ip_text[3])
{
    int is_unique;
    char *sender;
    char *another_sender;

    for (int i = 0; i < received; ++i)
    {
        is_unique = 1;
        sender = senders_ip_text[i];

        for (int j = 0; j < i; ++j)
        {
            another_sender = senders_ip_text[j];

            if (!strcmp(sender, another_sender))
            {
                is_unique = 0;
                break;
            }
        }

        if (is_unique)
        {
            printf("%s ", sender);
        }
    }
}