/**
 * @file ftps-server.c
 * @author Canciu Ionut - Cristian and Ilias Kalaitzidis
 * @brief 
 * @version 0.1
 * @date 2022-04-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "ftps-server.h"

int load_configuration(const int argc, char *argv[], struct server_config_t *config)
{
    if (argc == 1) 
    {
        char *config_file = CONFIG_FILE;
        printf("[+]Using `%s` file\n", config_file);
        /* Use parse_config_file to parse the config file */
        if (parse_config(config, config_file) == -1) 
        {
            perror("[-]Failed to parse config file\n");
            return EXIT_FAILURE;
        }
    } 
    else if (argc == 2 && strcmp(argv[1], "-default") == 0)
    {
        printf("[+]Using default configuration\n");
        config->num_threads = DEFAULT_NUM_THREADS;
        config->max_connections = DEFAULT_MAX_CONNECTIONS;
        config->ip = DEFAULT_IP;
        config->port = DEFAULT_PORT;
        config->reuse_socket = REUSE_SOCKET;
        config->home_dir = DEFAULT_HOME_DIR;
        config->cert_file = DEFAULT_CERT_FILE;
        config->key_file = DEFAULT_KEY_FILE;
    }
    else 
    {
        printf("[-]Invalid arguments\n");
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int start_server(struct server_config_t *config)
{   
    printf("[+]Starting server...\n");

    struct thread_pool_t *pool;
    pool = malloc(sizeof(thread_pool_t));

    if (init_thread_pool(pool, config->num_threads) == -1)
    {
        perror("[-]Error initializing thread pool");
        return EXIT_FAILURE;
    }
    struct server_state_t *state;
    state = malloc(sizeof(server_state_t));
    state->running = 1;
    state->active_connections = 0;

    /* Initialize OpenSSL */
    init_openssl();

    /* Setting up algorithms needed by TLS */
    state->ctx = create_context();
    /* Specify the certificate and private key to use */
    configure_context(state->ctx);
    /* Create socket */
    state->sock = create_control_socket(config->ip, config->port, config->reuse_socket);
    
    /* Use SSL session caching mode */
    SSL_CTX_set_session_cache_mode(state->ctx, SSL_SESS_CACHE_CLIENT);

    /* Start listening allowing a queue of up to max_connections pending connection */
    if (listen(state->sock, config->max_connections) < 0) {
	    perror("[-]Unable to listen");
	    exit(EXIT_FAILURE);
    }

    printf("[+]Server started\n");
    
    #ifdef DEBUG
        printf("[+]Server listening on %s:%d\n", config->ip, config->port);
    #endif

    printf("[+]Waiting for connections...\n");

    signal(SIGINT, sig_handler);

    /* Wait for connections */
    while(!stop) 
    {   
        printf("[/]Listening...\n");
        if (wait_client(state->sock, state->ctx) == -1) 
        {
            perror("[-]Service untrapped error\n");
            return EXIT_FAILURE;
        }
        state->active_connections++;
        printf("[\\]Client connected - new count: %d\n", state->active_connections);
    }

    close(state->sock);
    SSL_CTX_free(state->ctx);
    cleanup_openssl();
    printf("[+]Server stopped\n");
    return EXIT_SUCCESS;
}

int wait_client(int sock, SSL_CTX *ctx)
{   
    int client_sock;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    SSL *ssl;

    /* Accept client connection */
    if ((client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_len)) == -1) 
    {
        perror("[-]Failed to accept client connection\n");
        return EXIT_FAILURE;
    } 

    printf("[->]Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    /* Create SSL connection */
    if ((ssl = SSL_new(ctx)) == NULL) 
    {
        perror("[-]Failed to create SSL connection\n");
        return EXIT_FAILURE;
    }
 
    /* Associate the socket descriptor with the SSL context */
    if (!SSL_set_fd(ssl, client_sock)) 
    {
        perror("[-]Failed to set socket descriptor\n");
        return EXIT_FAILURE;
    }

    printf("[<-]SSL connection created\n");


    /* Perform SSL Handshake on the SSL connection */
    if (SSL_accept(ssl) == -1) 
    {
        perror("[-]Failed to accept SSL connection\n");
        ERR_print_errors_fp(stderr);
        return EXIT_FAILURE;
    }

    printf("[->]SSL connection accepted\n");

    /* Assign client socket to client struct */
    client_t *client = malloc(sizeof(client_t));
    client->sockfd = client_sock;
    client->ctx = ctx;
    client->ssl = ssl;
    /* Allocate memory for ip */
    client->ip = malloc(sizeof(char) * INET_ADDRSTRLEN);
    /* Set ip */
    inet_ntop(AF_INET, &(client_addr.sin_addr), client->ip, INET_ADDRSTRLEN);
    client->port = ntohs(client_addr.sin_port);
    client->active = 1;
    
    pid_t pid;

    /* Create thread to handle client */
    if ((pid = fork()) == -1)
    {
        perror("[-]Failed to fork\n");
        return EXIT_FAILURE;
    }
    else if (pid == 0)
    {
        /* Child process */
        handle_client(client);
    }

}

int handle_client(const struct client_t * client)
{   
    printf("\t[+][%s:%d]Handling client...\n", client->ip, client->port);

    /* writes num bytes from the buffer reply into the 
    * specified ssl connection
    */
    DIR *dp;
    int first = 0;
    struct dirent *entry;
    struct stat statbuf;
    int l = 0;
    int client_connected = 1;

    while(client_connected)
    {

        if(first == 0)
        {
            printf("\t[<-][%s:%d]Sending welcome message...\n", client->ip, client->port);
            SSL_write(client->ssl, "220 Service ready for new user\n\r", strlen("220 Service ready for new user\r\n"));
            first=1;
            chdir(dir);
        }

        char buff[255] = {};
        char username[255] = {};
        char * delim = " ";
        SSL_read(client->ssl, buff, sizeof(buff));
        printf("\t[->][%s:%d]Received: %s", client->ip, client->port, buff);

        char *command = strtok(buff, delim);
        char *arg = strtok(NULL, delim);

        if(command == NULL)
        {
            //SSL_write(ssl, "500 Syntax error\r\n", strlen("500 Syntax error\r\n"));
            first = 0;
            break;
        }
        if (strcmp(command, "USER") == 0 || strcmp(command, "USER\r\n") == 0) 
        {
                char rep[] = "331 User name ok, need password\r\n";
                SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "PASS") == 0 || strcmp(command, "PASS\r\n") == 0)
        {
            char rep[] = "230 User logged in, proceed\r\n";
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "SYST\r\n") == 0 || strcmp(command, "SYST") == 0)
        {
            char rep[] = "215 UNIX Type: L8\r\n";
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "FEAT\r\n") == 0 || strcmp(command, "FEAT") == 0)
        {
                
            char rep[] = "211 UNIX Type: L8\r\n";
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if (strcmp(command, "PASV\r\n") == 0 || strcmp(command, "PASV") == 0) 
        {
            srand(time(NULL));
            int p1 = 128 + (rand() % 64);
            int p2 = rand() % 0xff;
            dataport = (p1*256)+ p2;
            char rep[46];
            sprintf(rep, "227 Entering Passive Mode (127,0,0,1,%d,%d)\r\n", p1, p2);
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "PBSZ") == 0 || strcmp(command, "PBSZ\r\n") == 0)
        {
            char rep[] = "200 PBSZ set to 0\r\n";
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "PROT") == 0 || strcmp(command, "PROT\r\n") == 0)
        {
            char rep[] = "200 PROT now Private\r\n";
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "PWD\r\n") == 0 || strcmp(command, "PWD") == 0)
        {
            char rep[50];
            sprintf(rep, "257 %s\r\n", dir);
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "CWD") == 0 || strcmp(command, "CWD\r\n") == 0)
        {
            char d[100];
            if(arg[1] != '.') 
            { 
                int i = 0, c = 0;
                int form = 0;
                for(; i < strlen(arg); i++)
                {
                    if (isalnum(arg[i]) || arg[i] == '/' || arg[i] == '_' || arg[i] == '-')
                    {
                    d[c] = arg[i];
                    c++;
                    if(arg[i] == '/')
                        form ++;
                    }
                }   

                d[c] = '/';
                d[c+1] = '\0';
                fflush(stdout);
                if(form > 1){
                    d[c] = '\0';
                    strcpy(dir, d);
                }
                else
                strcat(dir, d);
            }
            else
            {
                char *e;
                int index;

                e = strrchr(dir, '/');
                index = (int)(e - dir);
                dir[index] = '\0';
                e = strrchr(dir, '/');
                index = (int)(e - dir);
                dir[index+1] = '\0';
            }
                
            char rep[] = "250 changed directory\r\n";
            SSL_write(client->ssl, rep, strlen(rep));     
        }
        else if(strcmp(command, "TYPE") == 0 || strcmp(command, "TYPE\r\n") == 0)
        {
            char rep[] = "200 Type set to A\r\n";
            SSL_write(client->ssl, rep, strlen(rep));
        }
        else if(strcmp(command, "LIST") == 0 || strcmp(command, "LIST\r\n") == 0)
        {
	        chdir(dir);
            if ((dp = opendir(dir)) == NULL )
            {
                perror(dir);
                return 0;
            }
            int sock2 = create_socket(dataport,1);
            char rep2[] = "150 Here comes the directory listing.\r\n";
            SSL_write(client->ssl, rep2, strlen(rep2));   
            while(1) {
                struct sockaddr_in addr2;
                uint len2 = sizeof(addr2);
                SSL *ssl2;

                int client2 = accept(sock2, (struct sockaddr*)&addr2, &len2);
                if (client2 < 0) {
                    perror("Unable to accept");
                    exit(EXIT_FAILURE);
                }

                /* creates a new SSL structure which is needed to hold the data 
                * for a TLS/SSL connection
                */ 
                ssl2 = SSL_new(client->ctx);
                SSL_set_fd(ssl2, client2);

                /* wait for a TLS/SSL client to initiate a TLS/SSL handshake */
                if (SSL_accept(ssl2) <= 0) {
                    ERR_print_errors_fp(stderr);
                }
                else{
                    while((entry = readdir(dp)) != NULL) {
                        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                            continue;
                        lstat(entry->d_name,&statbuf);
                        char rep2[300];
                        
                        char type[20];;
                        sprintf(type, "%ld", statbuf.st_size);
                        if (S_ISDIR(statbuf.st_mode)){
                            strcpy(type,"<DIR>");
                        }
                        char buff[20];
                        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&statbuf.st_mtime));
                        
                        sprintf(rep2, "%s %s %s\r\n",buff,type, entry->d_name);

                        SSL_write(ssl2, rep2, strlen(rep2));  
                    }

                    SSL_free(ssl2);
                    close(client2);
                    break;
                }  
            }
            close(sock2);
            char rep[] = "200 Command okay\r\n";
            SSL_write(client->ssl, rep, strlen(rep));

        }
        else if(strcmp(command, "RETR") == 0 || strcmp(command, "RETR\r\n") == 0)
        {
            char rep2[] = "150 Opening data connection.\r\n";
            SSL_write(client->ssl, rep2, strlen(rep2));

            int sock2 = create_socket(dataport,1); 
            while(1) {
                struct sockaddr_in addr2;
                uint len2 = sizeof(addr2);
                SSL *ssl2;

                int client2 = accept(sock2, (struct sockaddr*)&addr2, &len2);
                if (client2 < 0) {
                    perror("Unable to accept");
                    exit(EXIT_FAILURE);
                }

                /* creates a new SSL structure which is needed to hold the data 
                * for a TLS/SSL connection
                */ 
                ssl2 = SSL_new(client->ctx);
                SSL_set_fd(ssl2, client2);

                /* wait for a TLS/SSL client to initiate a TLS/SSL handshake */
                if (SSL_accept(ssl2) <= 0) {
                    ERR_print_errors_fp(stderr);
                }
                else{

                    char d[100]; 
                    int i = 0, c = 0;
                    
                    for(; i < strlen(arg); i++)
                    {
                        if (isalnum(arg[i]) || arg[i] == '.' || arg[i] == '_' || arg[i] == '-')
                        {
                            d[c] = arg[i];
                            c++;
                        }
                    } 
                        d[c] = '\0';

                    char source[1024*1024*5];
                    FILE *fp = fopen(d, "rb");
                    if (fp != NULL) {
                        size_t newLen = fread(source, sizeof(char), 1024*1024, fp);
                        if ( ferror( fp ) != 0 ) {
                            fputs("Error reading file", stderr);
                        } else {
                            source[newLen++] = '\0'; /* Just to be safe. */
                        }

                        fclose(fp);

                    }else{
                        char rep[] = "550 Requested action not taken. File unavailable\r\n";
                        SSL_write(client->ssl, rep, strlen(rep));  
                        SSL_free(ssl2);
                        close(client2);
                        close(sock2);
                        break; 

                    }
                    SSL_write(ssl2, source, strlen(source));
                    char rep[] = "226 Transfer complete..\r\n";
                    SSL_write(client->ssl, rep, strlen(rep));    

                    SSL_free(ssl2);
                    close(client2);
                    close(sock2);
                    break; 
                }
            }
        }
        else if(strcmp(command, "STOR") == 0 || strcmp(command, "STOR\r\n") == 0)
        {

            char rep2[] = "150 Opening data connection.\r\n";
            SSL_write(client->ssl, rep2, strlen(rep2));

            int sock2 = create_socket(dataport,1); 
            while(1) {
                struct sockaddr_in addr2;
                uint len2 = sizeof(addr2);
                SSL *ssl2;

                int client2 = accept(sock2, (struct sockaddr*)&addr2, &len2);
                if (client2 < 0) {
                    perror("Unable to accept");
                    exit(EXIT_FAILURE);
                }

                /* creates a new SSL structure which is needed to hold the data 
                * for a TLS/SSL connection
                */ 
                ssl2 = SSL_new(client->ctx);
                SSL_set_fd(ssl2, client2);

                /* wait for a TLS/SSL client to initiate a TLS/SSL handshake */
                if (SSL_accept(ssl2) <= 0) {
                    ERR_print_errors_fp(stderr);
                }
                else{

                    char d[100]; 
                    int i = 0, c = 0;
                    
                    for(; i < strlen(arg); i++)
                    {
                        if (isalnum(arg[i]) || arg[i] == '.' || arg[i] == '_' || arg[i] == '-')
                        {
                            d[c] = arg[i];
                            c++;
                        }
                    } 
                        d[c] = '\0';

                    char source[1024*1024*5] = {};
                    int sizeoffile=0;
                    char chunk[16384] ={};
                    while(1){
                        int readk = SSL_read(ssl2, chunk, sizeof(chunk));
                        if(readk <= 0){
                            break;

                        }
                        printf("%d lll\n", readk);
                        strncat(source, chunk,readk);
                        sizeoffile += readk;
                    }
                    
                    printf("%d\n", sizeoffile);
                    FILE *f = fopen(d, "wb");
                    if (f == NULL)
                    {
                        printf("Error opening file!\n");
                        exit(1);
                    }
                    fwrite(source,sizeoffile,1,f);
                    fclose(f);
                    char rep[] = "226 Transfer complete..\r\n";
                    SSL_write(client->ssl, rep, strlen(rep));    

                    SSL_free(ssl2);
                    close(client2);
                    close(sock2);
                    break; 
                }
            }
        }
        else if (strcmp(command, "MKD\r\n") == 0 || strcmp(command, "MKD") == 0)
        {

            chdir(dir);
            char d[100]; 
            int i = 0, c = 0;
            int slashes=0;
            for(; i < strlen(arg); i++)
            {
                if (isalnum(arg[i]) || arg[i] == '.' || arg[i] == '_' || arg[i] == '-')
                {
                    d[c] = arg[i];
                    c++;
                }
            } 
                d[c] = '\0';

            int result = mkdir(d, 0777);
            if(result==0){
                char rep[] = "257 Directory created.\r\n";
                SSL_write(client->ssl, rep, strlen(rep));
            }
            else{
                char rep[] = "550 Failed to create directory.\r\n";
                SSL_write(client->ssl, rep, strlen(rep));
            }
        }
        else if (strcmp(command, "DELE\r\n") == 0 || strcmp(command, "DELE") == 0)
        {

            chdir(dir);
            char d[100]; 
            int i = 0, c = 0;
            int slashes=0;
            for(; i < strlen(arg); i++)
            {
                if ((isalnum(arg[i]) || arg[i] == '.' || arg[i] == '_' || arg[i] == '-' || arg[i] == '/' ))
                {
                    d[c] = arg[i];
                    c++;
                }
            } 
                d[c] = '\0';
            printf("%s\n", d);
            if (remove(d) == 0) {
                char rep[] = "250 Succesfully deleted.\r\n";
                SSL_write(client->ssl, rep, strlen(rep));
            } else {
                char rep[] = "550 Failed to delete.\r\n";
                SSL_write(client->ssl, rep, strlen(rep));
            }
        }
        else 
        {
            char rep[] = "502 Command not implemented\r\n";
            SSL_write(client->ssl, rep, strlen(rep));    
        }
        // client_connected = 0;
    }
    return 0;
}

int main(int argc, char *argv[])
{   
    print_welcome();

    struct server_config_t *config;
    config = malloc(sizeof(struct server_config_t));

    if (load_configuration(argc, argv, config) != 0) {
        printf("[-]Error generating configuration\n");
        return EXIT_FAILURE;
    }

    #ifdef DEBUG
        /* Print config */
        print_config(config);
    #endif

    pid_t pid;
    int status;

    /* Create child process */
    if ((pid = fork()) == -1) 
    {
        perror("[-]Failed to fork\n");
        return EXIT_FAILURE;
    } 
    else if (pid == 0) 
    {
        /* Child process */
        /* Start the server */
        if (start_server(config) == -1) 
        {
            perror("[-]Failed to start server\n");
            return EXIT_FAILURE;
        } 
    } 
    else 
    {
        /* Parent process TODO: Make custom handlers */
        if (waitpid(pid, &status, 0) == -1) 
        {
            perror("[-]Failed to wait for child process\n");
            return -1;
        } 
        else if (WIFEXITED(status)) 
        {
            printf("[+]Server process exited with status %d\n", WEXITSTATUS(status));
        }
    }
}
