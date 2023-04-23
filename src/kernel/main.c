#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "../our_commons/utils.h"

#define PORT "7723"

int main() {
  int socket_kernel = init_socket(LOCALHOST, PORT);
  //  int socket_console = esperar_cliente(socket_kernel);
  //  recibir_mensaje(socket_console);
}
