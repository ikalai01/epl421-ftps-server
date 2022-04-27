int wait_client(int sock, SSL_CTX *ctx) 
{   
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
    int client, pid, bytes_read;
    bool signal_stop = false;

    while(!signal_stop) {
        SSL *ssl;
        printf("[+]Waiting for client...\n");
        if((client = accept(sock, (struct sockaddr*) &client_address,&len)) < 0) 
        {
            perror("[-]Accept error.");
            exit(EXIT_FAILURE);
        }

        char buffer[BSIZE];
        Command *cmd = malloc(sizeof(Command));
        State *state = malloc(sizeof(State));

        if((pid = fork()) < 0) 
        {
            perror("[-]Fork error.");
            exit(EXIT_FAILURE);
        }

        memset(buffer, 0, BSIZE);

        if (pid == 0)
        {
            printf("[/]Child process created with pid %d.\n", getpid());
            printf("[+]New connection received from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            char buffer[BSIZE];

            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, client);

            if (SSL_accept(ssl) <= 0) 
            {
                printf("[-]SSL accept error.\n");
                ERR_print_errors_fp(stderr);
            }

            /*  if TLS/SSL handshake was successfully completed, a TLS/SSL connection has been established */
            else
            {
                char welcome[BSIZE] = "220 ";

                if(strlen(welcome_message) < BSIZE - 4)
                    strcat(welcome, welcome_message);
                else
                    strcat(welcome, "Welcome to the FTP service.");

                /* Write welcome message */
                strcat(welcome,"\n\r");

                printf("[+]Sending welcome message...\n");
                SSL_write(ssl, welcome, strlen(welcome));

                printf("[+]Welcome message sent.\n");

                printf("[+]Waiting for command...\n");

                /* Read commands from client */
                while ((bytes_read = SSL_read(ssl, buffer, BSIZE)) > 0) {
                    
                    signal(SIGCHLD, my_wait);

                    if(!(bytes_read > BSIZE))
                    {
                        buffer[BSIZE-1] = '\0';
                        printf("[$]User %s sent command: %s\n",(state->username == 0) ? "unknown" : state->username, buffer);
                        parse_command(buffer, cmd);
                        state->connection = client;


                        /* Ignore non-ascii char. Ignores telnet command */
                        if(buffer[0]<=127 || buffer[0]>=0)
                            response(cmd, state);
                        
                        memset(buffer,0,BSIZE);
                        memset(cmd,0,sizeof(cmd));

                    }
                    else
                        perror("[-]Error reading from client.");
                    
                }
                printf("[+]Closing connection...\n");
                /* free an allocated SSL structure */
                SSL_free(ssl);
                close(client);
                exit(EXIT_SUCCESS);
            }
        }
    }
}

int accept_connection(int socket)
{
  int addrlen = 0;
  struct sockaddr_in client_address;
  addrlen = sizeof(client_address);
  return accept(socket,(struct sockaddr*) &client_address,&addrlen);
}

int lookup_cmd(char *cmd)
{   
  const int cmdlist_count = sizeof(cmdlist_str) / sizeof(char *);
  return lookup(cmd, cmdlist_str, cmdlist_count);
}

int lookup(char *needle, const char **haystack, int count)
{   
  int i;
  for(i = 0; i < count; i++){
    if(strcmp(needle, haystack[i]) == 0) return i;
  }
  return -1;
}

void parse_command(char *cmdstring, Command *cmd)
{
  sscanf(cmdstring, "%s %s", cmd->command, cmd->arg);
}

void getip(int sock, int *ip)
{   
  socklen_t addr_size = sizeof(struct sockaddr_in);
  struct sockaddr_in addr;
  getsockname(sock, (struct sockaddr *)&addr, &addr_size);
 
  char* host = inet_ntoa(addr.sin_addr);
  sscanf(host,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);
}

void write_state(State *state)
{ 
  SSL_write(state->ssl, state->message, strlen(state->message));
}

void gen_port(Port *port)
{   
  srand(time(NULL));
  port->p1 = 128 + (rand() % 64);
  port->p2 = rand() % 0xff;
}

typedef struct Port
{
  int p1;
  int p2;
} Port;

typedef struct State
{
  /* Connection mode: NORMAL, SERVER, CLIENT */
  int mode;

  /* Is user loggd in? */
  int logged_in;

  /* Is this username allowed? */
  int username_ok;
  char *username;
  
  /* Response message to client e.g. 220 Welcome */
  char *message;

  /* Commander connection */
  int connection;

  /* Socket for passive connection (must be accepted later) */
  int sock_pasv;

  /* Transfer process id */
  int tr_pid;

  /* OpenSSL context */
  SSL_CTX *ctx;

  /* OpenSSL connection */
  SSL *ssl;

} State;


/* Command struct */
typedef struct Command
{
  char command[5];
  char arg[BSIZE];
} Command;

/**
 * Connection mode
 * NORMAL - normal connection mode, nothing to transfer
 * SERVER - passive connection (PASV command), server listens
 * CLIENT - server connects to client (PORT command)
 */
typedef enum conn_mode{ NORMAL, SERVER, CLIENT } conn_mode;

/* Commands enumeration */
typedef enum cmdlist 
{ 
  ABOR, CWD, DELE, LIST, MDTM, MKD, NLST, PASS, PASV,
  PORT, PWD, QUIT, RETR, RMD, RNFR, RNTO, SITE, SIZE,
  STOR, TYPE, USER, NOOP
} cmdlist;

/* String mappings for cmdlist */
static const char *cmdlist_str[] = 
{
  "ABOR", "CWD", "DELE", "LIST", "MDTM", "MKD", "NLST", "PASS", "PASV",
  "PORT", "PWD", "QUIT", "RETR", "RMD", "RNFR", "RNTO", "SITE", "SIZE",
  "STOR", "TYPE", "USER", "NOOP", "FEAT", "PBSZ", "PROT"
};

/* Valid usernames for anonymous ftp */
static const char *usernames[] = 
{
  "ftp","anonymous","public","anon","test","foo"
};


int lookup_cmd(char *cmd);
int lookup(char *needle, const char **haystack, int count);
void getip(int sock, int *ip);
int wait_client(int sock, SSL_CTX *ctx);
int create_socket(const int port, const int reuse);
void gen_port(Port *);
void parse_command(char *, Command *);
void write_state(State *);
int accept_connection(int);


/* Commands handle functions*/
void response(Command *, State *);
void ftp_user(Command *, State *);
void ftp_pass(Command *, State *);
void ftp_pwd(Command *, State *);
void ftp_cwd(Command *, State *);
void ftp_mkd(Command *, State *);
void ftp_rmd(Command *, State *);
void ftp_pasv(Command *, State *);
void ftp_list(Command *, State *);
void ftp_retr(Command *, State *);
void ftp_stor(Command *, State *);
void ftp_dele(Command *, State *);
void ftp_size(Command *, State *);
void ftp_quit(State *);
void ftp_type(Command *, State *);
void ftp_abor(State *);

void my_wait(int);
void str_perm(int, char *);
