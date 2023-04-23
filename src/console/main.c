#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "../our_commons/utils.h"

int main() {
  int socket_cliente = crear_conexion(LOCALHOST, "7723");
  enviar_mensaje("mensaje de consola", socket_cliente);
}
