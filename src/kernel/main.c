#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "../our_commons/utils.h"
#include <pthread.h>

ptrNodo pilaNEW_FIFO;
ptrNodo pilaREADY_FIFO;

void* machine_states(void* ptr)
{
	STATES state = NEW;

	while(true)
	{
		switch(state)
		{
			case NEW:
				 {
					PCB* pcb = pop(&pilaNEW_FIFO);
					if (pcb != NULL)
						push(&pilaREADY_FIFO, pcb);
				 }
				 break;
			case READY: break;
			case EXEC: break;
			case EXIT: break;
			case BLOCK: break;
		}
	}
}

void* atender_cliente(void* socket_cliente)
{
	int socket_console = *((int *)socket_cliente);

	PCB pcb;
	push(&pilaNEW_FIFO, &pcb);

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
	enviar_mensaje_KERNEL("asd", socket_cliente_memory);
}

void* conectarse_cpu(void* ptr)
{
	t_config* config = iniciar_config();

	char* IP_CPU 		= config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU 	= config_get_string_value(config, "PUERTO_CPU");

	int socket_client = crear_conexion(IP_CPU, PUERTO_CPU);
	enviar_mensaje("hola cpu soy kernel", socket_client);
}

int main()
{
	t_config* config = iniciar_config();

	char* IP_KERNEL 		= config_get_string_value(config, "IP_KERNEL");
	char* PUERTO_KERNEL 	= config_get_string_value(config, "PUERTO_KERNEL");

	logger = log_create("./../log.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	pthread_t thread_memory, thread_cpu, pthread_machine_states;

	pthread_create(&thread_memory, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread_memory);

	pthread_create(&thread_cpu, NULL, (void*) conectarse_cpu, NULL);
	pthread_detach(thread_cpu);

	pthread_create(&pthread_machine_states, NULL, (void*) machine_states, NULL);
	pthread_detach(pthread_machine_states);

	int socket_servidor = init_socket(IP_KERNEL, PUERTO_KERNEL);

	while (1) {
	   pthread_t thread;
	   int *socket_cliente = malloc(sizeof(int));
	   *socket_cliente = accept(socket_servidor, NULL, NULL);
	   pthread_create(&thread, NULL, (void*) atender_cliente, socket_cliente);
	   pthread_detach(thread);
	}

	return EXIT_SUCCESS;
}

