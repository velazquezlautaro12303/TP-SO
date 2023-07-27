#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "my_commons.h"
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>
#include <commons/temporal.h>

ptrNodo pilaNEW_FIFO;
ptrNodo pilaREADY_FIFO;
ptrNodo pilaBLOCKED_FIFO;
ptrNodo pilaBLOCKED_FILESYSTEM_FIFO;
ptrNodo* pilaBLOCKED;
pthread_mutex_t mutex_pila_NEW, mutex_pila_READY, mutex_pila_BLOCKED, mutex_pila_BLOCKED_FILESYSTEM;
op_code opcode;
t_log* logger;
PCB* pcb;
char** RECURSOS;
char** INSTANCIAS_RECURSOS;
sem_t semaphore_MEMORY_1, semaphore_MEMORY_2;
sem_t semaphore_CPU_1, semaphore_CPU_2;
sem_t semaphore_FILESYSTEM_1, semaphore_FILESYSTEM_2;
int CANTIDAD_RECURSOS;
t_list* tabla_de_archivos;
uint32_t ESTIMACION_INICIAL;
double HRRN_ALFA; 
uint8_t ALGORITMO_PLANIFICACION;

INODO_PROCESO* buscarArchivo(Node_FCB* node_FCB, char *nameFile) 
{
	for (int i = 0; i < pcb->tablaArchivos.cant; i++)
	{
		if (strcmp(node_FCB->inodo.nombre_del_archivo, nameFile) == 0) 
		{
			return &(node_FCB->inodo);
		}
		node_FCB = node_FCB->next;
	}
	return NULL;
}

static int estaElArchivoEnLaTablaGlobal(char *nombre_del_archivo) {
	for (int i = 0; i < tabla_de_archivos->elements_count; i++)
	{
		INODO_GLOBAL* ptr = list_get(tabla_de_archivos, i);
		if (strcmp(ptr->nombre_del_archivo, nombre_del_archivo) == 0) {
			return i;
		}
	}
	return -1;
}

static void cantRecursos(t_config* config) {
	CANTIDAD_RECURSOS = 1;
	char* recursos 		= config_get_string_value(config, "RECURSOS");
	for (int i = 0; i < strlen(recursos); i++) {
		if (recursos[i] == ',') {
			CANTIDAD_RECURSOS++;
		}
	}
}

static int existeRecurso(char *recurso) 
{
	for (int i = 0; i < CANTIDAD_RECURSOS; i++) {
		if (strcmp(RECURSOS[i], recurso) == 0) {
			return i;
		}
	}
	return -1;
}

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
					pcb = pop(&pilaNEW_FIFO);
					pthread_mutex_unlock(&mutex_pila_NEW);

					if (pcb != NULL && grado_de_multiprogramacion())
					{
						opcode = INIT_PROCESS;
						sem_post(&semaphore_MEMORY_1);
						sem_wait(&semaphore_MEMORY_2);
						opcode = MENSAJE;
						pcb->tiempoLlegadaReady = time(NULL);
						pthread_mutex_lock(&mutex_pila_READY);
						push(&pilaREADY_FIFO, pcb);
						pthread_mutex_unlock(&mutex_pila_READY);
						log_info(logger, "PID: %i - Estado Anterior: NEW - Estado Actual: READY", pcb->PID);
					}
					
					state = READY;
				} break;
			case READY:
				{
					pthread_mutex_lock(&mutex_pila_READY);
					if (ALGORITMO_PLANIFICACION == 1) {
						pcb = pop(&pilaREADY_FIFO);
					} else if (ALGORITMO_PLANIFICACION == 2) {
						ptrNodo pila = pilaREADY_FIFO;
						time_t time_now = time(NULL);
						double est_prox_rafaga = 0;
						int pos = 0, i = 0;

						while (pila != NULL) {
							log_error(logger, "pila es Distinto de NULL");
							log_error(logger, "time_now = %ld", time_now);
							log_error(logger, "pila->info->tiempoLlegadaReady = %ld", pila->info->tiempoLlegadaReady);
							log_error(logger, "estimadoProxRafaga = %ld", pila->info->estimadoProxRafaga);
							time_t aux = time_now - pila->info->tiempoLlegadaReady;
							time_t est_prox_rafaga_PCB = (pila->info->estimadoProxRafaga + aux) / pila->info->estimadoProxRafaga;
							log_error(logger, "est_prox_rafaga_PCB = %ld", est_prox_rafaga_PCB);
							if (est_prox_rafaga < est_prox_rafaga_PCB) {
								est_prox_rafaga = est_prox_rafaga_PCB;
								pos = i;
								log_error(logger, "pos = %i", pos);
							}
							pila = pila->sig;
							i++;
						}
						for (i = 0; i < pos; i++)
						{
							PCB* pcb_aux = pop(&pilaREADY_FIFO);
							push(&pilaREADY_FIFO, pcb_aux);
						}
						pcb = pop(&pilaREADY_FIFO);
					}
					pthread_mutex_unlock(&mutex_pila_READY);
					if (pcb != NULL) {
						pcb->tiempoLlegadaReady = time(NULL);
						state = EXEC;
						log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", pcb->PID);
					} else {
						state = NEW;	
					}
				} break;
			case EXEC:
				{
					sem_wait(&semaphore_CPU_2);
					if (opcode == _EXIT) {
						opcode = MENSAJE;
						sem_post(&semaphore_CPU_2);
						state = EXIT;
						log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: EXIT", pcb->PID);
						break;	
					} else if (opcode == YIELD) {
						log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", pcb->PID);
						pcb->estimadoProxRafaga = pcb->estimadoProxRafaga * (1 - HRRN_ALFA) + HRRN_ALFA * (time(NULL) - pcb->tiempoLlegadaReady);
						pcb->tiempoLlegadaReady = time(NULL);
						pthread_mutex_lock(&mutex_pila_READY);
						push(&pilaREADY_FIFO, pcb);
						pthread_mutex_unlock(&mutex_pila_READY);
						sem_post(&semaphore_CPU_2);
						state = READY;
						opcode = MENSAJE;
						break;
					} else if (opcode == WAIT) {
						int posRecurso = 0;
						opcode = MENSAJE;
						sem_post(&semaphore_CPU_2);
						if ((posRecurso = existeRecurso(pcb->registerCPU.R[2])) >= 0) {
							if (atoi(INSTANCIAS_RECURSOS[posRecurso]) > 0) {
								sprintf(INSTANCIAS_RECURSOS[posRecurso], "%i", atoi(INSTANCIAS_RECURSOS[posRecurso])-1);
								log_info(logger, "PID: %i - Wait: %s - Instancias: %s", pcb->PID, pcb->registerCPU.R[2], INSTANCIAS_RECURSOS[posRecurso]);
							} else {
								push(pilaBLOCKED + posRecurso, pcb);
								log_error(logger, "pushe");
								log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCK", pcb->PID);
								state = READY;
							}
						} else {
							log_error(logger, "No se encontro recurso = %s", pcb->registerCPU.R[2]);
							state = EXIT;
						}
						break;
					} else if (opcode == SIGNAL) {
						int posRecurso = 0;
						opcode = MENSAJE;
						sem_post(&semaphore_CPU_2);
						if ((posRecurso = existeRecurso(pcb->registerCPU.R[2])) >= 0) {
							if (atoi(INSTANCIAS_RECURSOS[posRecurso]) == 0) {
								PCB* pcb_Recurso;
								if ((pcb_Recurso = pop(pilaBLOCKED + posRecurso)) != NULL) {
									pthread_mutex_lock(&mutex_pila_READY);
									push(&pilaREADY_FIFO, pcb_Recurso);
									pthread_mutex_unlock(&mutex_pila_READY);
								} else {
									sprintf(INSTANCIAS_RECURSOS[posRecurso], "%i", atoi(INSTANCIAS_RECURSOS[posRecurso])+1);
								}
							} else {
								sprintf(INSTANCIAS_RECURSOS[posRecurso], "%i", atoi(INSTANCIAS_RECURSOS[posRecurso])+1);
							}
							log_info(logger, "PID: %i - Signal: %s - Instancias: %s", pcb->PID, pcb->registerCPU.R[2], INSTANCIAS_RECURSOS[posRecurso]);
						} else {
							log_error(logger, "No se encontro recurso = %s", pcb->registerCPU.R[2]);
							state = EXIT;
						}
						break;
					} else if (opcode == BLOCKED) {
						opcode = MENSAJE;
						pcb->timer = temporal_create();
						sem_post(&semaphore_CPU_2);
						log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCK", pcb->PID);
						pthread_mutex_lock(&mutex_pila_BLOCKED);
						push(&pilaBLOCKED_FIFO, pcb);
						pthread_mutex_unlock(&mutex_pila_BLOCKED);
						state = READY;
						break;
					} else if (opcode == CREATE_SEGMENT || opcode == DELETE_SEGMENT) {
						sem_post(&semaphore_MEMORY_1);
						sem_wait(&semaphore_MEMORY_2);
						opcode = MENSAJE;
					} else if (opcode == SEGMENT_FAULT) {
						log_info(logger, "PID: %i - Error SEG_FAULT- Segmento: %i - Offset: %i - Tamaño: %i", pcb->PID, *((int*)(pcb->registerCPU.R[0])), *((int *)(pcb->registerCPU.R[1])), pcb->tablaSegmentos[*((int *)(pcb->registerCPU.R[0]))].lenSegmentoDatos);
					} else if (opcode == F_OPEN) {
						int pos;
						if ((pos = estaElArchivoEnLaTablaGlobal(pcb->registerCPU.R[2])) >= 0) {
							INODO_GLOBAL* inodo_global = list_get(tabla_de_archivos, pos);
							inodo_global->cantProcesos++;
							log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCK", pcb->PID);
							push(&(inodo_global->pilaFILE_FIFO), pcb);
							sem_post(&semaphore_CPU_2);
							state = READY;
							opcode = MENSAJE;
							break;
						} else {
							sem_post(&semaphore_FILESYSTEM_1);
							sem_wait(&semaphore_FILESYSTEM_2);
							if (opcode == FILE_NO_EXIST) {
								opcode = FILE_CREATE;
								sem_post(&semaphore_FILESYSTEM_1);
								sem_wait(&semaphore_FILESYSTEM_2);
							}
							INODO_GLOBAL *inodo = malloc(sizeof(INODO_GLOBAL));
							inodo->cantProcesos = 1;
							strcpy(inodo->nombre_del_archivo, pcb->registerCPU.R[2]);
							inodo->pilaFILE_FIFO = NULL;
							list_add(tabla_de_archivos, inodo);
						}
						opcode = MENSAJE;
						INODO_PROCESO* inodo_proceso = malloc(sizeof(INODO_PROCESO));
						inodo_proceso->ptr_Archivo = 0;
						strcpy(inodo_proceso->nombre_del_archivo, pcb->registerCPU.R[2]);
						insert_INODO(&(pcb->tablaArchivos.fcb), inodo_proceso);
						pcb->tablaArchivos.cant++;
						log_info(logger, "PID: %i - Abrir Archivo: %s", pcb->PID, pcb->registerCPU.R[2]);
					} else if (opcode == F_CLOSE) {
						int pos;
						if ((pos = estaElArchivoEnLaTablaGlobal(pcb->registerCPU.R[2])) >= 0) {
							Node_FCB** node_FCB = &(pcb->tablaArchivos.fcb);
							for (int i = 0; i < pcb->tablaArchivos.cant; i++)
							{
								if (strcmp((*node_FCB)->inodo.nombre_del_archivo, pcb->registerCPU.R[2]) == 0) 
								{
									Node_FCB* tmp = *node_FCB;
									*node_FCB = (*node_FCB)->next;
									free(tmp);
									break;
								}
								node_FCB = &((*node_FCB)->next);
							}
							pcb->tablaArchivos.cant--;
							INODO_GLOBAL* inodo_global = list_get(tabla_de_archivos, pos);
							if (inodo_global->cantProcesos == 1) {
								list_remove(tabla_de_archivos, pos);
							} else {
								PCB* pcb = pop(&(inodo_global->pilaFILE_FIFO));
								inodo_global->cantProcesos--;
								pthread_mutex_lock(&mutex_pila_READY);
								push(&pilaREADY_FIFO, pcb);
								pthread_mutex_unlock(&mutex_pila_READY);
							}
							log_info(logger, "PID: %i - Cerrar Archivo: %s", pcb->PID, pcb->registerCPU.R[2]);
						}
					} else if (opcode == F_SEEK) {
						Node_FCB* node = pcb->tablaArchivos.fcb;
						for (int i = 0; i < pcb->tablaArchivos.cant; i++) {
							if (strcmp(node->inodo.nombre_del_archivo, pcb->registerCPU.R[2]) == 0) {
								node->inodo.ptr_Archivo = atoi(pcb->registerCPU.R[3]);
								break;
							}
							node = node->next;
						}
						log_info(logger, "PID: %i - Actualizar puntero Archivo: %s - Puntero %i", pcb->PID, node->inodo.nombre_del_archivo, node->inodo.ptr_Archivo);
					} else if (opcode == F_TRUNCATE || opcode == F_READ || opcode == F_WRITE) {
						if (opcode == F_TRUNCATE) 
							log_info(logger, "PID: %i - Archivo: %s - Tamaño: %i", pcb->PID, pcb->registerCPU.R[2], atoi(pcb->registerCPU.R[3]));
						else if (opcode == F_READ || opcode == F_WRITE) {
							INODO_PROCESO* inodo = buscarArchivo(pcb->tablaArchivos.fcb, pcb->registerCPU.R[2]);
							pcb->generico[0] = inodo->ptr_Archivo;
							if (opcode == F_READ)
								log_info(logger, "PID: %i - Leer Archivo: %s - Puntero %i - Dirección Memoria %i - Tamaño %s", pcb->PID, pcb->registerCPU.R[2], inodo->ptr_Archivo, *((int*)(pcb->registerCPU.R[6])), pcb->registerCPU.R[4]);
							else if (opcode == F_WRITE) 
								log_info(logger, "PID: %i - Escribir Archivo: %s - Puntero %i - Dirección Memoria %i - Tamaño %s", pcb->PID, pcb->registerCPU.R[2], inodo->ptr_Archivo, *((int*)(pcb->registerCPU.R[6])), pcb->registerCPU.R[4]);
						}
						log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCK", pcb->PID);
						pthread_mutex_lock(&mutex_pila_BLOCKED_FILESYSTEM);
						push(&pilaBLOCKED_FILESYSTEM_FIFO, pcb);
						pthread_mutex_unlock(&mutex_pila_BLOCKED_FILESYSTEM);
						sem_post(&semaphore_CPU_2);
						state = READY;
						opcode = MENSAJE;
						break;
					}
					sem_post(&semaphore_CPU_1);
				} break;
			case EXIT: 
				{
					opcode = MENSAJE;
					log_info(logger, "Finaliza el proceso %i - Motivo:", pcb->PID);
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
	op_code opcode;
	int socket_console = *((int *)socket_cliente);
	
	PCB* pcb = recibirPCB(socket_console, &opcode);	

	pcb->estimadoProxRafaga = ESTIMACION_INICIAL;

	log_info(logger, "Se crea el proceso %i en NEW", pcb->PID);

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

	while (true)
	{
		sem_wait(&semaphore_MEMORY_1);

		enviarPCB(pcb, socket_cliente_memory, opcode);
		pcb = recibirPCB(socket_cliente_memory, &opcode);

		sem_post(&semaphore_MEMORY_2);
	}

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
		sem_wait(&semaphore_CPU_1);
		enviarPCB(pcb, socket_client, MENSAJE);
		pcb = recibirPCB(socket_client, &opcode);
		sem_post(&semaphore_CPU_2);
	}

	return NULL;
}

void* conectarse_filesystem_pila(void* ptr) 
{
	int socket_client = *((int *)ptr);
	while(true)
	{
		pthread_mutex_lock(&mutex_pila_BLOCKED_FILESYSTEM);
		PCB* pcb_FILESYSTEM = pop(&pilaBLOCKED_FILESYSTEM_FIFO);
		pthread_mutex_unlock(&mutex_pila_BLOCKED_FILESYSTEM);

		if (pcb_FILESYSTEM != NULL) 
		{
			enviarPCB(pcb_FILESYSTEM, socket_client, *((op_code *)(pcb_FILESYSTEM->registerCPU.R[5])));
			free(pcb_FILESYSTEM);
			pcb_FILESYSTEM = recibirPCB(socket_client, &opcode);
			log_info(logger, "PID: %i - Estado Anterior: BLOCK - Estado Actual: READY", pcb_FILESYSTEM->PID);
			pcb_FILESYSTEM->estimadoProxRafaga = pcb_FILESYSTEM->estimadoProxRafaga * (1 - HRRN_ALFA) + HRRN_ALFA * (time(NULL) - pcb_FILESYSTEM->tiempoLlegadaReady);
			pcb_FILESYSTEM->tiempoLlegadaReady = time(NULL);
			log_error(logger, "tiempoLlegadaReady = %ld", pcb_FILESYSTEM->tiempoLlegadaReady);
			pthread_mutex_lock(&mutex_pila_READY);
			push(&pilaREADY_FIFO, pcb_FILESYSTEM);
			pthread_mutex_unlock(&mutex_pila_READY);
		}
	}
}

void* conectarse_filesystem(void* ptr)
{
	pthread_t thread_filesystem;
	
	t_config* config = iniciar_config();
	
	char* IP_FILESYSTEM 		= config_get_string_value(config, "IP_FILESYSTEM");
	char* PUERTO_FILESYSTEM 	= config_get_string_value(config, "PUERTO_FILESYSTEM");

	int socket_client = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);

	pthread_create(&thread_filesystem, NULL, (void*) conectarse_filesystem_pila, &socket_client);
	pthread_detach(thread_filesystem);

	while(true)
	{
		sem_wait(&semaphore_FILESYSTEM_1);
		enviarPCB(pcb, socket_client, opcode);
		pcb = recibirPCB(socket_client, &opcode);
		sem_post(&semaphore_FILESYSTEM_2);
	}

	return NULL;
}

void* state_blocked(void* ptr) 
{
	PCB* pcb;
	
	while (true)
	{
		pthread_mutex_lock(&mutex_pila_BLOCKED);
		pcb = pop(&pilaBLOCKED_FIFO);
		pthread_mutex_unlock(&mutex_pila_BLOCKED);

		if (pcb != NULL) {
			// log_error(logger, "pcb->registerCPU.R[2] = %s temporal_gettime(pcb->timer) = %li", pcb->registerCPU.R[2], temporal_gettime(pcb->timer));
			if (temporal_gettime(pcb->timer) > atoi(pcb->registerCPU.R[2]) * 1000) {
				log_error(logger, "Cumplio condicion de timer");
				temporal_destroy(pcb->timer);
				log_info(logger, "PID: %i - Estado Anterior: BLOCK - Estado Actual: READY", pcb->PID);
				pcb->estimadoProxRafaga = pcb->estimadoProxRafaga * (1 - HRRN_ALFA) + HRRN_ALFA * (time(NULL) - pcb->tiempoLlegadaReady);
				pcb->tiempoLlegadaReady = time(NULL);
				log_error(logger, "tiempoLlegadaReady = %ld", pcb->tiempoLlegadaReady);
				pthread_mutex_lock(&mutex_pila_READY);
				push(&pilaREADY_FIFO, pcb);
				pthread_mutex_unlock(&mutex_pila_READY);
			} else {
				// log_error(logger, "Vuelvo a pushear");
				pthread_mutex_lock(&mutex_pila_BLOCKED);
				push(&pilaBLOCKED_FIFO, pcb);
				pthread_mutex_unlock(&mutex_pila_BLOCKED);
			}
		}
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	t_config* config = iniciar_config();

    pilaNEW_FIFO = NULL;
	pilaBLOCKED_FILESYSTEM_FIFO = NULL;
	pcb = NULL;
	opcode = MENSAJE;

	logger = log_create("./../../log.log", "Kernel", 1, LOG_LEVEL_TRACE);
	log_error(logger, "%s: %i", argv[0], getpid());

	tabla_de_archivos		= list_create();

	char* IP_KERNEL				 		= config_get_string_value(config, "IP_KERNEL");
	char* PUERTO_KERNEL 				= config_get_string_value(config, "PUERTO_KERNEL");
	RECURSOS 							= config_get_array_value(config, "RECURSOS");
	INSTANCIAS_RECURSOS					= config_get_array_value(config, "INSTANCIAS_RECURSOS");
	ESTIMACION_INICIAL 					= config_get_int_value(config, "ESTIMACION_INICIAL");
	HRRN_ALFA		 					= config_get_double_value(config, "HRRN_ALFA");
	char* ALGORITMO_PLANIFICACION_AUX 	= config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if (strcmp(ALGORITMO_PLANIFICACION_AUX, "FIFO") == 0) {
		ALGORITMO_PLANIFICACION = 1;
	} else if (strcmp(ALGORITMO_PLANIFICACION_AUX, "HRRN") == 0) {
		ALGORITMO_PLANIFICACION = 2;
	}

	cantRecursos(config);

	pilaBLOCKED = malloc(CANTIDAD_RECURSOS * sizeof(ptrNodo));

	for (int i = 0; i < CANTIDAD_RECURSOS; i++) {
		pilaBLOCKED[i] = NULL;
	}

	sem_init(&semaphore_CPU_1, 0, 0);
	sem_init(&semaphore_CPU_2, 0, 1);
	sem_init(&semaphore_MEMORY_1, 0, 0);
	sem_init(&semaphore_MEMORY_2, 0, 0);
	sem_init(&semaphore_FILESYSTEM_1, 0, 0);
	sem_init(&semaphore_FILESYSTEM_2, 0, 0);

	pthread_t thread_memory, thread_cpu, thread_filesystem, pthread_machine_states, pthread_machine_states_blocked;

	pthread_mutex_init(&mutex_pila_NEW, NULL);
	pthread_mutex_init(&mutex_pila_READY, NULL);
	pthread_mutex_init(&mutex_pila_BLOCKED, NULL);
	pthread_mutex_init(&mutex_pila_BLOCKED_FILESYSTEM, NULL);

	pthread_create(&thread_memory, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread_memory);

	pthread_create(&thread_cpu, NULL, (void*) conectarse_cpu, NULL);
	pthread_detach(thread_cpu);

	pthread_create(&thread_filesystem, NULL, (void*) conectarse_filesystem, NULL);
	pthread_detach(thread_filesystem);

	pthread_create(&pthread_machine_states, NULL, (void*) machine_states, NULL);
	pthread_detach(pthread_machine_states);

	pthread_create(&pthread_machine_states_blocked, NULL, (void*) state_blocked, NULL);
	pthread_detach(pthread_machine_states_blocked);

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
