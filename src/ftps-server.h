#include "lib/common.h"
#include "lib/support.h"

int start_server(struct server_config_t *config);
int wait_client(int sock, SSL_CTX *ctx);
int handle_client(const struct client_t * client);
int load_configuration(const int argc, char *argv[], struct server_config_t *config);