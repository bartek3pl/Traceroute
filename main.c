#include "traceroute.h"

#include <stdio.h>
#include <arpa/inet.h>

int validate_args(int argc)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: ./traceroute [address]\n");
        return EXIT_FAILURE;
    }

    return 1;
}

int validate_address(char *address, struct sockaddr_in *recipient)
{
    recipient->sin_family = AF_INET;

    int inet_pton_status = inet_pton(AF_INET, address, &recipient->sin_addr);
    if (!inet_pton_status)
    {
        fprintf(stderr, "Provided address is not a valid network address: %s\n", address);
        return EXIT_FAILURE;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    int validate_args_status = validate_args(argc);
    if (!validate_args_status)
    {
        return EXIT_FAILURE;
    }

    char *address = argv[1];
    struct sockaddr_in recipient;

    int validate_address_status = validate_address(address, &recipient);
    if (validate_address_status == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    // printf("source address: %s\n", address);

    int sockfd = create_socket();
    if (sockfd == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    traceroute(sockfd, &recipient);

    return 0;
}