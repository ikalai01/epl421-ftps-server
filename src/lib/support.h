#include <stdio.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int first=0;
char dir[30] = "/ftphome/";
int dataport;
volatile sig_atomic_t stop = 0;

void print_usage(void)
{   
    printf("Usage: `./server` or `./server -default`\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Prints the welcome message 
 */
void print_welcome(void)
{   
    printf("Welcome to the FTPS server!\n");
    printf("[+]Initializing...\n");
}

/**
 * @brief Prints the exit message
 */
void print_exit(void)
{   
    printf("[+]Exiting...\n");
}

/**
 * @brief Waits for user input before exiting
 */
void wait_exit(void) 
{   
    printf("[/]Press any key to exit... ");
    char c;
    scanf("%c", &c);
    exit(0);
}

/**
 * @brief Initializes OpenSSL
 */
void init_openssl(void)
{   
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

/**
 * @brief Creates a new SSL context
 * @return SSL context
 */
SSL_CTX *create_context(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    /* The actual protocol version used will be negotiated to the 
     * highest version mutually supported by the client and the server.      
     * The supported protocols are SSLv3, TLSv1, TLSv1.1 and TLSv1.2. 
     */
    method = SSLv23_server_method();

    /* creates a new SSL_CTX object as framework to establish TLS/SSL or   
     * DTLS enabled connections. It initializes the list of ciphers, the 
     * session cache setting, the callbacks, the keys and certificates, 
     * and the options to its default values
     */
    ctx = SSL_CTX_new(method);
    if (!ctx) {
	    perror("[-]Unable to create SSL context");
	    ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }

    return ctx;
}

/**
 * @brief Configures the SSL context
 * @param ctx SSL context
 */
void configure_context(SSL_CTX *ctx)
{   
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert using dedicated pem files */
    if (SSL_CTX_use_certificate_file(ctx, "certificates/server-cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "certificates/server-key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }
}

/**
 * @brief Cleans up the SSL context 
 */
void cleanup_openssl(void)
{   
    EVP_cleanup();
}

/**
 * @brief Handles zombie processes
 * 
 * @param signum 
 */
void my_wait(int signum)
{   
  int status;
  wait(&status);
}

/** 
 * Converts permissions to string. e.g. rwxrwxrwx 
 * @param perm Permissions mask
 * @param str_perm Pointer to string representation of permissions
 */
void str_perm(int perm, char *str_perm)
{
  int curperm = 0;
  int flag = 0;
  int read, write, exec;
  
  /* Flags buffer */
  char fbuff[3];

  read = write = exec = 0;
  
  int i;
  for(i = 6; i>=0; i-=3){
    /* Explode permissions of user, group, others; starting with users */
    curperm = ((perm & ALLPERMS) >> i ) & 0x7;
    
    memset(fbuff,0,3);
    /* Check rwx flags for each*/
    read = (curperm >> 2) & 0x1;
    write = (curperm >> 1) & 0x1;
    exec = (curperm >> 0) & 0x1;

    //sprintf(fbuff,"%c%c%c",read?'r':'-' ,write?'w':'-', exec?'x':'-');
    strcat(str_perm,fbuff);

  }
}

/******************************************************************************/
/**
 * @brief This function is used to parse the configuration file.
 * config file is given under `config.conf`
 * config file is in the format of:
 * <SETTING_NAME>=<value>
 * - Comments are allowed using `#`
 * - Unknow settings are ignored
 *
 * @param num_threads - number of threads to be used
 * @param max_connections - max number of connections to be handled
 * @param port - port number to be used
 * @param reuse_socket - whether to reuse the socket or not
 * @param homedir - home directory of the server
 * @param cert_file - certificate file to be used
 * @param key_file - key file to be used
 * @return int - 0 on success, -1 on failure
 */
int parse_config(server_config_t *config, char *filename)
{   
    printf("[+]Parsing config file...\n");
    FILE *fp;
    char line[MAX_LINE_LEN];
    char *setting;
    char *value;
    char *saveptr;
    int line_num = 0;
    
    /* open the config file */
    if ((fp = fopen(filename, "r")) == NULL) {
        printf("[-]Failed to open config file\n");
        return -1;
    }

    /* read the config file line by line amd print tokens */
    while(fgets(line, MAX_LINE_LEN, fp) != NULL) {
        line_num++;
        /* ignore comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        /* remove trailing newline */
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        /* parse the line */
        setting = strtok_r(line, "=", &saveptr);
        value = strtok_r(NULL, "=", &saveptr);

        #ifdef DEBUG
            /* print line number and line */
            printf("[+]Line %d: %s", line_num, line);
            /* print tokens */
            printf("[+]Using %s = %s\n", setting, value);
        #endif

        /* check if the setting is valid */
        if (strcmp(setting, "NUM_THREADS") == 0) 
        {
            config->num_threads = atoi(value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %d\n", setting, config->num_threads);
            #endif
        }
        else if (strcmp(setting, "MAX_CONNECTIONS") == 0) 
        {
            config->max_connections = atoi(value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %d\n", setting, config->max_connections);
            #endif
        }
        else if (strcmp(setting, "IP") == 0)
        {
            /* allocate memory for the ip */
            config->ip = (char *)malloc(strlen(value) + 1);
            if (config->ip == NULL) {
                printf("[-]Failed to allocate memory for ip\n");
                return -1;
            }
            strcpy(config->ip, value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %s\n", setting, config->ip);
            #endif
        }
        else if (strcmp(setting, "PORT") == 0) 
        {
            config->port = atoi(value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %d\n", setting, config->port);
            #endif
        }
        else if (strcmp(setting, "REUSE_SOCKET") == 0) 
        {
            config->reuse_socket = atoi(value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %d\n", setting, config->reuse_socket);
            #endif
        }
        else if (strcmp(setting, "HOME_DIR") == 0) 
        {
            /* allocate memory for the home directory */
            config->home_dir = malloc(strlen(value) + 1);
            if (config->home_dir == NULL) {
                printf("[-]Failed to allocate memory for home directory pointer\n");
                return -1;
            }
            /* copy the home directory */
            strcpy(config->home_dir, value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %s\n", setting, config->home_dir);
            #endif
        }
        else if (strcmp(setting, "CERT_FILE") == 0) 
        {
            /* allocate memory for the certificate file */
            config->cert_file = malloc(strlen(value) + 1);
            if (config->cert_file == NULL) {
                printf("[-]Failed to allocate memory for certificate file pointer\n");
                return -1;
            }
            /* copy the certificate file */
            strcpy(config->cert_file, value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %s\n", setting, config->cert_file);
            #endif
        }
        else if (strcmp(setting, "KEY_FILE") == 0) 
        {
            /* allocate memory for the key file */
            config->key_file = malloc(strlen(value) + 1);
            if (config->key_file == NULL) {
                printf("[-]Failed to allocate memory for key file pointer\n");
                return -1;
            }
            /* copy the key file */
            strcpy(config->key_file, value);
            #ifdef DEBUG
                printf("[+]Using setting %s = %s\n", setting, config->key_file);
            #endif
        }
        else {
            /* unknown setting */
            printf("[?]Unknown setting %s\n", setting);
        }
    }

    /* close the config file */
    fclose(fp);
    printf("[+]Config file parsed successfully\n");
    return 0;
}

/**
 * @brief Function to print the configiuration
 * 
 * @param config - pointer to server_config_t struct
 * @return int 
 */
void print_config(struct server_config_t *config)
{   
    /* print config */
    printf("[+]Config:\n");
    printf("[+]NUM_THREADS: %d\n", config->num_threads);
    printf("[+]MAX_CONNECTIONS: %d\n", config->max_connections);
    printf("[+]IP: %s\n", config->ip);
    printf("[+]PORT: %d\n", config->port);
    printf("[+]REUSE_SOCKET: %d\n", config->reuse_socket);
    printf("[+]HOME_DIR: %s\n", config->home_dir);
    printf("[+]CERT_FILE: %s\n", config->cert_file);
    printf("[+]KEY_FILE: %s\n", config->key_file);
}


int create_control_socket(const char *ip, const int port, const int reuse)
{   
    printf("[+]Creating socket...\n");
    int sock;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-]Unable to create socket");
        exit(EXIT_FAILURE);
    }

    /* Address can be reused instantly after program exits */
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    /* bind serv information to s socket */
    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
	    perror("[-]Unable to bind");
	    exit(EXIT_FAILURE);
    }

    #ifdef DEBUG
        printf("[+]Socket created on port %d\n", port);
    #endif

    return sock;
}

int create_socket(const int port, const int reuse)
{   
    printf("[+]Creating socket...\n");
    int sock;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-]Unable to create socket");
        exit(EXIT_FAILURE);
    }

    /* Address can be reused instantly after program exits */
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    /* bind serv information to s socket */
    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
	    perror("[-]Unable to bind");
	    exit(EXIT_FAILURE);
    }

    printf("[+]Socket created on port %d\n", port);

    /* start listening allowing a queue of up to 5 pending connection */
    if (listen(sock, 5) < 0) {
	    perror("[-]Unable to listen");
	    exit(EXIT_FAILURE);
    }

    printf("[+]Listening...\n");

    return sock;
}


void handle_cwd(const struct client_t * client, char * arg)
{
    char d[100];
    if(arg[1] != '.'){   
    int i = 0, c = 0;
    
    for(; i < strlen(arg); i++)
    {
        if (isalnum(arg[i]))
        {
            d[c] = arg[i];
            c++;
        }
    } 
        d[c] = '/';
        d[c+1] = '\0';
        strcat(dir, d);
    }
    else{
        strcpy(d,"..");
    }
    
    strcpy(dir,d);
    chdir(dir);
    char rep[] = "250 changed directory\r\n";
    SSL_write(client->ssl, rep, strlen(rep));  
}

int init_thread_pool(thread_pool_t *pool, int num_threads)
{
    printf("[+]Initializing thread pool with %d threads...\n", num_threads);
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        return -1;
    }
    pool->num_threads = num_threads;
    pool->max_connections = DEFAULT_MAX_CONNECTIONS;
    pool->num_connections = 0;
    pool->shutdown = 0;
    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->cond), NULL);

    printf("[+]Thread pool initialized\n");
    return 0;
}

void cleanup_thread_pool(thread_pool_t *pool)
{
    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
}

void sig_handler(int signum)
{
    if(signum == SIGINT){
        printf("\n");
        printf("[+]Server shutting down...\n");
        stop = 1;
        exit(EXIT_SUCCESS);
    }
}
