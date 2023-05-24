#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "../our_commons/utils.h"
#include <commons/config.h>
#include <sys/types.h>
#include <unistd.h>

int main() 
{
	t_config* config = iniciar_config();
	char* ip 		 = config_get_string_value(config, "IP_KERNEL");
	char* puerto 	 = config_get_string_value(config, "PUERTO_KERNEL");
	
	int socket_cliente = crear_conexion(ip, puerto);
	
	PCB pcb;
	pcb.PID = getpid();
	enviar_structura(&pcb, socket_cliente, sizeof(PCB));
	
	return EXIT_SUCCESS;
}
