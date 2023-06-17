#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include "my_commons.h"
#include <pthread.h>
#include <semaphore.h>

t_list* list_segments = NULL;
void* memory;
t_log* logger;

sem_t semaphore_CPU, semaphore_KERNEL, semaphore_FILESYSTEM;
int RETARDO_MEMORIA, RETARDO_COMPACTACION, CANT_SEGMENTOS, TAM_SEGMENTO_0;

void* atender_cliente_KERNEL(void* socket_cliente);
void* atender_cliente_CPU(void* socket_cliente);
void* atender_cliente_FILESYSTEM(void* socket_cliente);
void* memory_init(void*);

int main() 
{
	t_config* config = iniciar_config();

	char* ip 		= config_get_string_value(config, "IP_MEMORIA");
	char* puerto 	= config_get_string_value(config, "PUERTO_MEMORIA");

	logger = log_create("./../../log.log", "Memory", 1, LOG_LEVEL_TRACE);
	int socket_servidor = init_socket(ip, puerto);

	sem_init(&semaphore_CPU, 0, 0);
	sem_init(&semaphore_FILESYSTEM, 0, 0);
	sem_init(&semaphore_KERNEL, 0, 0);

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) memory_init, NULL);
	pthread_detach(thread);

	while (1) 
	{
		pthread_t thread;
		int *socket_cliente = malloc(sizeof(int));
		*socket_cliente = accept(socket_servidor, NULL, NULL);
		int cod_op = recibir_operacion(*socket_cliente);
	   
		switch (cod_op)
		{
			case CPU:
				sem_post(&semaphore_CPU);
				pthread_create(&thread, NULL, atender_cliente_CPU, socket_cliente);
				pthread_detach(thread);
				break;

			case KERNEL:
				sem_post(&semaphore_KERNEL);
				pthread_create(&thread, NULL, atender_cliente_KERNEL, socket_cliente);
				pthread_detach(thread);
				break;

			case FILESYSTEM:
				sem_post(&semaphore_FILESYSTEM);
				pthread_create(&thread, NULL, atender_cliente_FILESYSTEM, socket_cliente);
				pthread_detach(thread);
				break;
		}
	}

	return EXIT_SUCCESS;
}

void* atender_cliente_KERNEL(void* socket_cliente)
{
	op_code opcode;
	PCB* pcb;
	int socket_console = *((int *)socket_cliente);

	pcb = recibirPCB(socket_console, opcode);

	switch (opcode)
	{
		case INIT_PROCESS:
			pcb->tablaSegmentos = malloc(CANT_SEGMENTOS * sizeof(TABLE_SEGMENTS));
			TABLE_SEGMENTS* segment_0 = (TABLE_SEGMENTS*)list_get(list_segments, 0);
			pcb->tablaSegmentos[0].direccionBase = segment_0->direccionBase;
			pcb->tablaSegmentos[0].lenSegmentoDatos = segment_0->lenSegmentoDatos;
			enviarPCB(pcb, socket_console, INIT_PROCESS);
			break;
	}

	return NULL;
}

void* atender_cliente_CPU(void* socket_cliente)
{
	puts("atender_cliente_CPU");
	return NULL;
}

void* atender_cliente_FILESYSTEM(void* socket_cliente)
{
	puts("atender_cliente_FILESYSTEM");
	return NULL;
}

void* memory_init(void*)
{
	sem_wait(&semaphore_KERNEL);
	sem_wait(&semaphore_CPU);
	sem_wait(&semaphore_FILESYSTEM);

	logger = log_create("./../../log.log", "Memory", 1, LOG_LEVEL_TRACE);

	t_config* config = iniciar_config();

	int TAM_MEMORIA 		= config_get_int_value(config, "TAM_MEMORIA");
	TAM_SEGMENTO_0 			= config_get_int_value(config, "TAM_SEGMENTO_0");
	CANT_SEGMENTOS 			= config_get_int_value(config, "CANT_SEGMENTOS");
	RETARDO_MEMORIA 		= config_get_int_value(config, "RETARDO_MEMORIA");
	RETARDO_COMPACTACION	= config_get_int_value(config, "RETARDO_COMPACTACION");

	memory = malloc(TAM_MEMORIA);

	TABLE_SEGMENTS* segment_0 = malloc(sizeof(TABLE_SEGMENTS));

	segment_0->direccionBase = memory;
	segment_0->lenSegmentoDatos = TAM_SEGMENTO_0;

	list_add(list_segments, segment_0);

	return NULL;	
}
