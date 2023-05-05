#include <stdio.h>
#include "../our_commons/utils.h"
#include <pthread.h>

void* atender_cliente(void* socket_cliente)
{
	int socket_console = *((int *)socket_cliente);
	int cod_op = recibir_operacion(socket_console);
	switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_console);
			break;
	}
}

void* conectarse_memory(void* ptr)
{
	t_config* config = iniciar_config();

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje_CPU("asd", socket_cliente_memory);
}

int main() {

	logger = log_create("./../log.log", "CPU", 1, LOG_LEVEL_DEBUG);

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread);

	t_config* config = iniciar_config();

	char* IP_CPU 		= config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU 	= config_get_string_value(config, "PUERTO_CPU");

	int socket_servidor = init_socket(IP_CPU, PUERTO_CPU);

	while(1)
	{
		pthread_t thread;
		int *socket_cliente = malloc(sizeof(int));
		*socket_cliente = accept(socket_servidor, NULL, NULL);
		pthread_create(&thread, NULL, (void*) atender_cliente, socket_cliente);
		pthread_detach(thread);
	}

	return EXIT_SUCCESS;
}
