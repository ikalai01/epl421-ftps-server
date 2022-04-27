#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024

void write_file(int sock)
{
    int n;
    FILE *fp;
    char *filename = "test1.txt";
    char data[SIZE];

    if ((fp = fopen(filename, "w")) == NULL) {
        perror("[-]Error in opening file.\n");
        exit(1);
    }

    while ((n = recv(sock, data, SIZE, 0)) > 0) {
        if (fwrite(data, sizeof(char), n, fp) != n) {
            perror("[-]Error in writing file.\n");
            exit(1);
        }
        bzero(data, SIZE);
    }

    printf("[+]File received.\n");
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int bind_e;

    int sockfd, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    char buffer[SIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[-]Error in socket creation.\n");
        exit(1);
    }

    printf("[+]Socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if((bind_e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)) {
        perror("[-]Error in binding.\n");
        exit(1);
    }

    printf("[+]Binding done.\n");

    if(listen(sockfd, 1) == -1) {
        perror("[-]Error in listening.\n");
        exit(1);
    }

    printf("[+]Listening...\n");

    sin_size = sizeof(client_addr);
    if((new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
        perror("[-]Error in accepting.\n");
        exit(1);
    }

    printf("[+]Connection accepted.\n");

    write_file(new_sock);

    printf("[+]Disconnecting from client.\n");

    close(sockfd);
    close(new_sock);

}