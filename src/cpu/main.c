#include <stdio.h>
#include "../our_commons/utils.h"
#include <pthread.h>

void* conectarse_memory(void* Nothing)
{
	t_config* config = iniciar_config();

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje_CPU("asd", socket_cliente_memory);
}

int main() {
	pthread_t thread;
	pthread_create(&thread,
					  NULL,
					  (void*) conectarse_memory,
					  NULL);
	pthread_detach(thread);

	while(1)
	{

	}

	return EXIT_SUCCESS;
}
