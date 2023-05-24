#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include "../our_commons/utils.h"
#include <pthread.h>

void* atender_cliente_KERNEL(void* socket_cliente);
void* atender_cliente_CPU(void* socket_cliente);
void* atender_cliente_FILESYSTEM(void* socket_cliente);

void * (*atender_cliente[3])(void*);

int main() 
{
	atender_cliente[0] = atender_cliente_CPU;
	atender_cliente[1] = atender_cliente_KERNEL;
	atender_cliente[2] = atender_cliente_FILESYSTEM;

	t_config* config = iniciar_config();

	char* ip 		= config_get_string_value(config, "IP_MEMORIA");
	char* puerto 	= config_get_string_value(config, "PUERTO_MEMORIA");

	logger = log_create("./../log.log", "Memory", 1, LOG_LEVEL_DEBUG);
	int socket_servidor = init_socket(ip, puerto);

	while (1) 
	{
	   pthread_t thread;
	   int *socket_cliente = malloc(sizeof(int));
	   *socket_cliente = accept(socket_servidor, NULL, NULL);
	   int cod_op = recibir_operacion(*socket_cliente);
	   if (cod_op <= 2 && cod_op >= 0) 
	   {
		   pthread_create(&thread, NULL, (void*) atender_cliente[cod_op], socket_cliente);
		   pthread_detach(thread);
	   }
	}

	return EXIT_SUCCESS;
}

void* atender_cliente_KERNEL(void* socket_cliente)
{
	puts("atender_cliente_KERNEL");
}

void* atender_cliente_CPU(void* socket_cliente)
{
	puts("atender_cliente_CPU");
}

void* atender_cliente_FILESYSTEM(void* socket_cliente)
{
	puts("atender_cliente_FILESYSTEM");
}
