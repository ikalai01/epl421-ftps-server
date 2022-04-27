#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024

void send_file(FILE *fp, int sockfd)
{
    char data[SIZE] = {0};

    while(fgets(data, SIZE, fp) != NULL)
    {
        if (send(sockfd, data, strlen(data), 0) == -1)
        {
            perror("[-]Error in sending file.\n");
            exit(1);
        }
        bzero(data, SIZE);
    }
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int connect_e;

    int sockfd, new_sock;
    struct sockaddr_in server_addr;
    FILE *fp;
    char *filename = "test.txt";

    if ((new_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[-]Error in socket creation.\n");
        exit(1);
    }

    printf("[+]Socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if((connect_e = connect(new_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) == -1) {
        perror("[-]Error in connecting.\n");
        exit(1);
    }

    printf("[+]Connection established.\n");

    if ((fp = fopen(filename, "r")) == NULL) {
        perror("[-]Error in opening file.\n");
        exit(1);
    }

    send_file(fp, new_sock);

    printf("[+]File sent.\n");

    fclose(fp);

    printf("[+]Disconnecting from server.\n");

    close(new_sock);

}