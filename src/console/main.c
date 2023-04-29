#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "../our_commons/utils.h"
#include <commons/config.h>

int main() {
	t_config* config = iniciar_config();
	char* ip 		= config_get_string_value(config, "IP_KERNEL");
	char* puerto 	= config_get_string_value(config, "PUERTO_KERNEL");
	int socket_cliente = crear_conexion(ip, puerto);
	enviar_mensaje("hola", socket_cliente);
	return EXIT_SUCCESS;
}
