#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "my_commons.h"
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

ptrNodo pilaNEW_FIFO;
ptrNodo pilaREADY_FIFO;
ptrNodo pilaBLOCKED_FIFO;
pthread_mutex_t mutex_pila_NEW, mutex_pila_READY;
op_code opcode;
t_log* logger;
PCB* pcb;
// sem_t semaphore;
sem_t semaphore_EXEC_1, semaphore_EXEC_2;

bool grado_de_multiprogramacion()
{
	return true;
}

void* machine_states(void* ptr)
{
	static STATES state = NEW;
	while(true)
	{
		switch(state)
		{
			case NEW:
				{
					pthread_mutex_lock(&mutex_pila_NEW);
					PCB* pcb = pop(&pilaNEW_FIFO);
					pthread_mutex_unlock(&mutex_pila_NEW);

					if (pcb != NULL && grado_de_multiprogramacion())
					{
						// pthread_mutex_lock(&mutex_pila_READY);
						push(&pilaREADY_FIFO, pcb);
						// pthread_mutex_unlock(&mutex_pila_READY);
					}
					
					state = READY;
				} break;
			case READY:
				{
					// pthread_mutex_lock(&mutex_pila_READY);
					pcb = pop(&pilaREADY_FIFO);
					// pthread_mutex_unlock(&mutex_pila_READY);
					state = pcb != NULL ? EXEC : NEW;
				} break;
			case EXEC:
				{
					sem_wait(&semaphore_EXEC_2);
					// log_info(logger, "Ejecutando.. PID = %i PC = %i\n", pcb->PID, pcb->PC);
					if (opcode == _EXIT) {
						sem_post(&semaphore_EXEC_2);
						state = EXIT;
						break;
					} else if (opcode == BLOCKED) {
						sem_post(&semaphore_EXEC_2);
						push(&pilaBLOCKED_FIFO, pcb);
						state = READY;
						break;
					}
					sem_post(&semaphore_EXEC_1);
				} break;
			case EXIT: 
				{
					log_info(logger, "Liberando PCB del PID = %i\n", pcb->PID);
					// free(pcb);
					state = NEW;
				} break;
			case BLOCK: break;
		}
	}
	return NULL;
}

void* atender_cliente(void* socket_cliente)
{
	int socket_console = *((int *)socket_cliente);

	PCB* pcb = recibirPCB(socket_console, &opcode);	

	pthread_mutex_lock(&mutex_pila_NEW);
	push(&pilaNEW_FIFO, pcb);
	pthread_mutex_unlock(&mutex_pila_NEW);

	return NULL;
}

void* conectarse_memory(void* ptr)
{
	t_config* config = iniciar_config();

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje_KERNEL("", socket_cliente_memory);

	return NULL;
}

void* conectarse_cpu(void* ptr)
{
	t_config* config = iniciar_config();

	char* IP_CPU 		= config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU 	= config_get_string_value(config, "PUERTO_CPU");

	int socket_client = crear_conexion(IP_CPU, PUERTO_CPU);

	while(true)
	{
		sem_wait(&semaphore_EXEC_1);
		enviarPCB(pcb, socket_client, MENSAJE);
		pcb = recibirPCB(socket_client, &opcode);
		sem_post(&semaphore_EXEC_2);
	}

	return NULL;
}



int main()
{
	t_config* config = iniciar_config();

    pilaNEW_FIFO = NULL;
	opcode = MENSAJE;

	char* IP_KERNEL 		= config_get_string_value(config, "IP_KERNEL");
	char* PUERTO_KERNEL 	= config_get_string_value(config, "PUERTO_KERNEL");

	logger = log_create("./../../log.log", "Kernel", 0, LOG_LEVEL_INFO);
	
	// sem_init(&semaphore, 0, 0);
	sem_init(&semaphore_EXEC_1, 0, 0);
	sem_init(&semaphore_EXEC_2, 0, 1);

	pthread_t thread_memory, thread_cpu, pthread_machine_states;

	pthread_mutex_init(&mutex_pila_NEW, NULL);
	pthread_mutex_init(&mutex_pila_READY, NULL);

	pthread_create(&thread_memory, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread_memory);

	pthread_create(&thread_cpu, NULL, (void*) conectarse_cpu, NULL);
	pthread_detach(thread_cpu);

	pthread_create(&pthread_machine_states, NULL, (void*) machine_states, NULL);
	pthread_detach(pthread_machine_states);

	int socket_servidor = init_socket(IP_KERNEL, PUERTO_KERNEL);

	while (1) 
	{
	   pthread_t thread;
	   int *socket_cliente = malloc(sizeof(int));
	   *socket_cliente = accept(socket_servidor, NULL, NULL);
	   pthread_create(&thread, NULL, (void*) atender_cliente, socket_cliente);
	   pthread_detach(thread);
	}
	
	return EXIT_SUCCESS;
}
