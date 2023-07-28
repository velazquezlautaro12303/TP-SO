#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include "my_commons.h"
#include <pthread.h>
#include <semaphore.h>

t_list* list_segments;
void* memory;
t_log* logger;

sem_t semaphore_CPU, semaphore_KERNEL, semaphore_FILESYSTEM;
int RETARDO_MEMORIA, RETARDO_COMPACTACION, CANT_SEGMENTOS, TAM_SEGMENTO_0, TAM_MEMORIA;
int cod_Algoritmo_Asignacion; // 1 first 2 BEST 3 WORST

void* atender_cliente_KERNEL(void* socket_cliente);
void* atender_cliente_CPU(void* socket_cliente);
void* atender_cliente_FILESYSTEM(void* socket_cliente);
void* memory_init(void*);

bool comparator(void* a, void* b)
{
	TABLE_SEGMENTS* _a = (TABLE_SEGMENTS*)a;
	TABLE_SEGMENTS* _b = (TABLE_SEGMENTS*)b;
	return _a->direccionBase < _b->direccionBase;
}

void compactar() 
{
	usleep(RETARDO_COMPACTACION*1000);
	t_list* list_sort = list_sorted(list_segments, comparator);

	for (int i = 0; i < list_sort->elements_count - 1; i++)
	{
		TABLE_SEGMENTS* unit_segment_1;
		TABLE_SEGMENTS* unit_segment_2;
		unit_segment_1 = list_get(list_sort, i);
		unit_segment_2 = list_get(list_sort, i+1);

		unit_segment_2->direccionBase = unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos;
	}

	for (int i = 0; i < list_sort->elements_count; i++)
	{
		TABLE_SEGMENTS* unit_segment_1 = list_get(list_sort, i);
		log_info(logger, "PID: %i - Segmento: %i - Base: %i - Tamaño %i", unit_segment_1->pid, unit_segment_1->idSegmento, unit_segment_1->direccionBase, unit_segment_1->lenSegmentoDatos);
	}
}

bool asignarMemoria(int espacio, int* dir) 
{
	t_list* list_sort = list_sorted(list_segments, comparator);

	bool condition = true;

	*dir = TAM_SEGMENTO_0;

	TABLE_SEGMENTS* unit_segment_1;
	TABLE_SEGMENTS* unit_segment_2;

	log_error(logger, "cod_Algoritmo_Asignacion = %i", cod_Algoritmo_Asignacion);
	log_error(logger, "TAMANIO MEMORIA = %i", TAM_MEMORIA);

	if (cod_Algoritmo_Asignacion == 1) 
	{
		for (int i = 0; i < list_sort->elements_count - 1; i++)
		{
			unit_segment_1 = list_get(list_sort, i);
			unit_segment_2 = list_get(list_sort, i+1);
			log_error(logger, "unit_segment_2->direccionBase = %i", unit_segment_2->direccionBase);
			log_error(logger, "unit_segment_1->direccionBase = %i", unit_segment_1->direccionBase);
			log_error(logger, "unit_segment_1->lenSegmentoDatos = %i", unit_segment_1->lenSegmentoDatos);
			log_error(logger, "-------------------------------------");
			if (unit_segment_2->direccionBase - (unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos) >= espacio) {
				*dir = unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos;
				log_error(logger, "dir = %i", *dir);
				break;
			} else if (i+1 == list_sort->elements_count - 1) {
				if (TAM_MEMORIA - (unit_segment_2->direccionBase + unit_segment_2->lenSegmentoDatos) >= espacio) {
					*dir = unit_segment_2->direccionBase + unit_segment_2->lenSegmentoDatos;
					log_error(logger, "dir = %i", *dir);
				} else {
					condition = false;
				}
			} 
		}
	} else if (cod_Algoritmo_Asignacion == 2) 
	{
		int pos = 0, fragmentacion_interna = TAM_MEMORIA;
		if (list_sort->elements_count > 1) {
			pos = -1;
		}
		for (int i = 0; i < list_sort->elements_count - 1; i++)
		{
			unit_segment_1 = list_get(list_sort, i);
			unit_segment_2 = list_get(list_sort, i+1);
			log_error(logger, "unit_segment_2->direccionBase = %i", unit_segment_2->direccionBase);
			log_error(logger, "unit_segment_1->direccionBase = %i", unit_segment_1->direccionBase);
			log_error(logger, "unit_segment_1->lenSegmentoDatos = %i", unit_segment_1->lenSegmentoDatos);
			log_error(logger, "-------------------------------------");
			int fragmentacion = unit_segment_2->direccionBase - (unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos) - espacio;
			log_debug(logger, "fragmentacion = %i\nfragmentacion interna = %i", fragmentacion, fragmentacion_interna);
			if (fragmentacion >= 0 && fragmentacion_interna >= fragmentacion) {
				fragmentacion_interna = fragmentacion;
				pos = i;
			}
			if (i+1 == list_sort->elements_count - 1) {
				fragmentacion = TAM_MEMORIA - (unit_segment_2->direccionBase + unit_segment_2->lenSegmentoDatos) - espacio;
				log_debug(logger, "______fragmentacion = %i\nfragmentacion interna = %i", fragmentacion, fragmentacion_interna);
				if (fragmentacion >= 0 && fragmentacion_interna >= fragmentacion) {
					pos = i+1;
				}
			}
		}
		if (pos == -1) {
			condition = false;
		} else {
			unit_segment_1 = list_get(list_sort, pos);
			*dir = unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos;
			log_error(logger, "se asigna dir = %i", *dir);
		}
	} else if (cod_Algoritmo_Asignacion == 3)
	{
		int pos = 0, fragmentacion_interna = 0;
		if (list_sort->elements_count > 1) {
			pos = -1;
		}
		for (int i = 0; i < list_sort->elements_count - 1; i++)
		{
			unit_segment_1 = list_get(list_sort, i);
			unit_segment_2 = list_get(list_sort, i+1);
			log_error(logger, "unit_segment_2->direccionBase = %i", unit_segment_2->direccionBase);
			log_error(logger, "unit_segment_1->direccionBase = %i", unit_segment_1->direccionBase);
			log_error(logger, "unit_segment_1->lenSegmentoDatos = %i", unit_segment_1->lenSegmentoDatos);
			log_error(logger, "-------------------------------------");
			int fragmentacion = unit_segment_2->direccionBase - (unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos) - espacio;
			if (fragmentacion >= 0 && fragmentacion_interna <= fragmentacion) {
				fragmentacion_interna = fragmentacion;
				pos = i;
			}
			if (i+1 == list_sort->elements_count - 1) {
				fragmentacion = TAM_MEMORIA - (unit_segment_2->direccionBase + unit_segment_2->lenSegmentoDatos) - espacio;
				if (fragmentacion >= 0 && fragmentacion_interna <= fragmentacion) {
					pos = i+1;
				} 
			}
		}
		if (pos == -1) {
			condition = false;
		} else {
			unit_segment_1 = list_get(list_sort, pos);
			*dir = unit_segment_1->direccionBase + unit_segment_1->lenSegmentoDatos;
			log_error(logger, "se asigna dir = %i", *dir);
		}
	}

	return condition;
}

int memoria_disponible()
{
	int tamanio = 0;
	TABLE_SEGMENTS* unit_segment;
	for (int i  = 0; i < list_size(list_segments); i++)
	{
		unit_segment = list_get(list_segments, i);
		tamanio += unit_segment->lenSegmentoDatos;
	}

	return TAM_MEMORIA - tamanio;
}

static int list_get_index_element_equal(t_list* list_segments, TABLE_SEGMENTS *segment) 
{
	int index = -1;
	for (int i = 0; i < list_size(list_segments); i++) {
		TABLE_SEGMENTS *segment_unit = list_get(list_segments, i);
		if (segment_unit->direccionBase == segment->direccionBase && segment_unit->lenSegmentoDatos == segment->lenSegmentoDatos) {
			index = i;
			break;							
		}
	}
	return index;
}

int main(int argc, char* argv[]) 
{
	list_segments = list_create();

	t_config* config 			= iniciar_config("./../../Memoria.config");
	char* ip 					= config_get_string_value(config, "IP_MEMORIA");
	char* puerto 				= config_get_string_value(config, "PUERTO_MEMORIA");
	char* ALGORITMO_ASIGNACION	= config_get_string_value(config, "ALGORITMO_ASIGNACION");
	RETARDO_MEMORIA 			= config_get_int_value(config, "RETARDO_MEMORIA");
	RETARDO_COMPACTACION 		= config_get_int_value(config, "RETARDO_COMPACTACION");
	
	if (strcmp(ALGORITMO_ASIGNACION, "FIRST") == 0) {
		cod_Algoritmo_Asignacion = 1;
	} else if (strcmp(ALGORITMO_ASIGNACION, "BEST") == 0) {
		cod_Algoritmo_Asignacion = 2;
	} else if (strcmp(ALGORITMO_ASIGNACION, "WORST") == 0) {
		cod_Algoritmo_Asignacion = 3;
	}

	logger = log_create("./../../log.log", "Memory", 1, LOG_LEVEL_TRACE);
	log_error(logger, "%s: %i", argv[0], getpid());

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
		op_code opcode = recibir_operacion(*socket_cliente);
	   
		switch (opcode)
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

			default: break;
		}
	}

	return EXIT_SUCCESS;
}

void* atender_cliente_KERNEL(void* socket_cliente)
{
	op_code opcode;
	PCB* pcb;
	int size;
	int socket_console = *((int *)socket_cliente);
	
	recibir_mensaje(socket_console, &size);
	
	while (true)
	{
		pcb = recibirPCB(socket_console, &opcode);
		
		switch (opcode)
		{
			case INIT_PROCESS:
				{				
					log_info(logger, "Creación de Proceso PID: %i", pcb->PID);
					pcb->tablaSegmentos = malloc(CANT_SEGMENTOS * sizeof(TABLE_SEGMENTS));
					TABLE_SEGMENTS* segment_0 = (TABLE_SEGMENTS*)list_get(list_segments, 0);
					for (int i = 0; i < CANT_SEGMENTOS; i++) {
						pcb->tablaSegmentos[i].direccionBase = 0;
						pcb->tablaSegmentos[i].lenSegmentoDatos = 0;
					}
					pcb->tablaSegmentos[0].direccionBase = segment_0->direccionBase;
					pcb->tablaSegmentos[0].lenSegmentoDatos = segment_0->lenSegmentoDatos;
					pcb->CANT_SEGMENTOS = CANT_SEGMENTOS;
				} break;

			case CREATE_SEGMENT:
			/**REVISAR IMPLEMENTACION CUANDO SE INTENTA ESCRIBIR FUERA DEL SEGMENTO EL BITMAP*/
				{
					int dir;
					int id_segment = atoi(pcb->registerCPU.R[2]);
					int tamanio = atoi(pcb->registerCPU.R[3]);
					int memoriaDisponible = memoria_disponible();
					if (memoriaDisponible > 0) {
						for (int i = 0; i < 2; i++)
						{
							if (asignarMemoria(tamanio, &dir)) {
								TABLE_SEGMENTS *segment = malloc(sizeof(TABLE_SEGMENTS));
								segment->direccionBase = dir;
								segment->lenSegmentoDatos = tamanio;
								segment->pid = pcb->PID;
								segment->idSegmento = id_segment;
								list_add(list_segments, segment);
								pcb->tablaSegmentos[id_segment].direccionBase = dir;
								pcb->tablaSegmentos[id_segment].lenSegmentoDatos = tamanio;
								log_info(logger, "PID: %i - Crear Segmento: %i - Base: %i - TAMAÑO: %i", pcb->PID, id_segment, pcb->tablaSegmentos[id_segment].direccionBase, pcb->tablaSegmentos[id_segment].lenSegmentoDatos);
								break;
							} else if (i == 0)
							{
								log_info(logger, "Solicitud de Compactación");
								compactar();
							}
						}
					} else {
						log_error(logger, "NO HAY ESPACIO EN LA MEMORIA");
					}
				} break;

			case DELETE_SEGMENT:
				{
					int id_segment = atoi(pcb->registerCPU.R[2]);
					int index = list_get_index_element_equal(list_segments, &(pcb->tablaSegmentos[id_segment]));
					if (index >= 0) {
						list_remove(list_segments, index);
						log_info(logger, "PID: %i - Eliminar Segmento: %i - Base: %i - TAMAÑO: %i", pcb->PID, id_segment, pcb->tablaSegmentos[id_segment].direccionBase, pcb->tablaSegmentos[id_segment].lenSegmentoDatos);
						pcb->tablaSegmentos[id_segment].direccionBase = 0;
						pcb->tablaSegmentos[id_segment].lenSegmentoDatos = 0;
					}
				} break;

			default:
				break;
		}
		enviarPCB(pcb, socket_console, MENSAJE);
	}

	return NULL;
}

void* atender_cliente_CPU(void* socket_cliente)
{
	int socket_console = *((int *)socket_cliente);
	op_code opcode;
	PCB* pcb;
	int size;
	recibir_mensaje(socket_console, &size);

	while (true)
	{
		pcb = recibirPCB(socket_console, &opcode);
		usleep(RETARDO_MEMORIA*1000);
		switch (opcode)
		{
			/*Para Opcode F_WRITE y F_READ*/
			case GET_DIR_FISICA:
				{
					TABLE_SEGMENTS* segment = &(pcb->tablaSegmentos[*((int*)(pcb->registerCPU.R[6]))]);
					int dirFisica = segment->direccionBase + *((int*)(pcb->registerCPU.R[7]));
					memcpy(pcb->registerCPU.R[6], &dirFisica, 4);
				} break;
			case MOV_OUT:
				{
					TABLE_SEGMENTS* segment = &(pcb->tablaSegmentos[*((int*)(pcb->registerCPU.R[0]))]);
					int dirFisica = segment->direccionBase + *((int*)(pcb->registerCPU.R[1]));
					int tamanio = size_reg(pcb->registerCPU.R[3]);
					memcpy(memory + dirFisica, pcb->registerCPU.R[4], tamanio);
					log_info(logger, "PID: %i - Acción: <ESCRIBIR> - Dirección física: %i - Tamaño: %i - Origen: <CPU>", pcb->PID, dirFisica, size_reg(pcb->registerCPU.R[3]));
				} break;

			case MOV_IN:
				{
					TABLE_SEGMENTS* segment = &(pcb->tablaSegmentos[*((int*)(pcb->registerCPU.R[0]))]);
					int dirFisica = segment->direccionBase + *((int*)(pcb->registerCPU.R[1]));
					int tamanio = size_reg(pcb->registerCPU.R[2]);
					memcpy(pcb->registerCPU.R[4], memory + dirFisica, tamanio);	
					log_info(logger, "PID: %i - Acción: <LEER> - Dirección física: %i - Tamaño: %i - Origen: <CPU>", pcb->PID, dirFisica, tamanio);
				} break;

			default: break;
		}
		enviarPCB(pcb, socket_console, opcode);
	}

	return NULL;
}

void* atender_cliente_FILESYSTEM(void* socket_cliente)
{
	int socket_console = *((int *)socket_cliente);
	op_code opcode;
	PCB* pcb;
	int size;
	recibir_mensaje(socket_console, &size);

	while (true) 
	{
		pcb = recibirPCB(socket_console, &opcode);
		usleep(RETARDO_MEMORIA*1000);
		switch (opcode)
		{
			case F_READ:
			{
				INFO_MEMORY *data = (INFO_MEMORY*)(pcb->generico);
				//log_debug(logger, "info = %c pos = %i\n", data->info, data->ptr);
				*((char*)(memory + data->ptr)) = data->info;
				log_info(logger, "PID: %i - Acción: <ESCRIBIR> - Dirección física: %i - Tamaño: %i - Origen: <FS>", pcb->PID, data->ptr, 1);
			} break;

			case F_WRITE:
			{
				int direccionFisica =  *((int*)(pcb->registerCPU.R[6]));
				//log_debug(logger, "info de la memoria dir %i = %c", direccionFisica, *((char*)(memory + direccionFisica)));
				// log_info(logger, "PID: %i - Acción: <LEER> - Dirección física: %i - Tamaño: %i - Origen: <FS>", pcb->PID, dirFisica, tamanio);
				memcpy(pcb->registerCPU.R[7], memory + direccionFisica, 1);
				log_info(logger, "PID: %i - Acción: <LEER> - Dirección física: %i - Tamaño: %i - Origen: <FS>", pcb->PID, direccionFisica, 1);
			} break;
			
			default:
				break;
		}

		enviarPCB(pcb, socket_console, opcode);
	}

	return NULL;
}

void* memory_init(void* ptr)
{
	sem_wait(&semaphore_KERNEL);
	sem_wait(&semaphore_CPU);
	sem_wait(&semaphore_FILESYSTEM);

	t_config* config = iniciar_config("./../../Memoria.config");

	TAM_MEMORIA 			= config_get_int_value(config, "TAM_MEMORIA");
	TAM_SEGMENTO_0 			= config_get_int_value(config, "TAM_SEGMENTO_0");
	CANT_SEGMENTOS 			= config_get_int_value(config, "CANT_SEGMENTOS");
	RETARDO_MEMORIA 		= config_get_int_value(config, "RETARDO_MEMORIA");
	RETARDO_COMPACTACION	= config_get_int_value(config, "RETARDO_COMPACTACION");

	memory = malloc(TAM_MEMORIA);

	TABLE_SEGMENTS* segment_0 = malloc(sizeof(TABLE_SEGMENTS));

	segment_0->direccionBase = 0;
	segment_0->lenSegmentoDatos = TAM_SEGMENTO_0;

	list_add(list_segments, segment_0);

	return NULL;
}
