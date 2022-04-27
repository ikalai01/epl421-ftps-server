#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>

#define CONFIG_FILE "./config/config.conf"

#define MAX_LINE_LEN 1024
#define DEFAULT_NUM_THREADS 10
#define DEFAULT_MAX_CONNECTIONS 1024 // max number of active tasks in the thread pool
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 30000
#define REUSE_SOCKET 1 // 1 to allow reuse of socket, 0 to not
#define DEFAULT_HOME_DIR "/home/ftp"
#define DEFAULT_CERT_FILE "./certificates/server-cert.pem"
#define DEFAULT_KEY_FILE "./certificates/server-cert.pem"

#ifndef BSIZE // Buffer size
  #define BSIZE 1024
#endif

typedef struct server_config_t {
    int num_threads;
    int max_connections;
    char *ip;
    int port;
    int reuse_socket;
    char *home_dir;
    char *cert_file;
    char *key_file;
} server_config_t;

typedef struct thread_pool_t {
    pthread_t *threads;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int num_threads;
    int max_connections;
    int num_connections;
    int shutdown;
} thread_pool_t;

typedef struct client_t {
    int sockfd;
    SSL_CTX *ctx;
    SSL *ssl;
    char *ip;
    int port;
    int active;
} client_t;

typedef struct server_state_t {
    int running;
    int active_connections;
    int sock;
    SSL_CTX *ctx;
} server_state_t;

/* Welcome message */
static char *welcome_message = "Service ready for new user";

/* OpenSSL functions */
void init_openssl(void);
SSL_CTX* create_context(void);
void configure_context(SSL_CTX *ctx);
void cleanup_openssl(void);

/* Server functions */
int start_server(struct server_config_t *config);
int create_control_socket(const char *ip, const int port, const int reuse);
int wait_client(int sock, SSL_CTX *ctx);
int handle_client(const struct client_t * client);
int init_thread_pool(thread_pool_t *pool, int num_threads);
void cleanup_thread_pool(thread_pool_t *pool);

/* Helper functions */
int parse_config(server_config_t *config, char *filename);
void print_config(server_config_t *config);
void print_welcome(void);
void print_exit(void);
void wait_exit(void);
void sig_handler(int signum);

