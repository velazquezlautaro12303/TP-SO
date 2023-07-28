#include <stdio.h>
#include "my_commons.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <commons/bitarray.h>
#include <math.h>

int RETARDO_ACCESO_BLOQUE;

typedef struct 
{
	uint32_t BLOCK_SIZE;
	uint32_t BLOCK_COUNT;
} SUPER_BLOCK;

t_log* logger;
int fd_fcb;
SUPER_BLOCK superBlocked;
t_bitarray* bitArrayMap;


typedef struct
{
	char nombre_del_archivo[32];
	uint32_t tamanio;
	uint32_t bloque_directo;
	uint32_t bloque_indirecto;
} FCB;

int asignarBit() {
	for (int z = 0; z < superBlocked.BLOCK_COUNT/8; z++) {
		if (bitarray_test_bit(bitArrayMap, z) == false) {
			bitarray_set_bit(bitArrayMap, z);
			log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 1", z);
			return z;
		}
	}
	return -1;
}

int tamanioFile(int fd)
{
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}

bool existeArchivo(char * nameArchivo) {
	FCB* fcb = malloc(sizeof(FCB));
	lseek(fd_fcb, 0, SEEK_SET);
	for (int i = 0; i < tamanioFile(fd_fcb)/sizeof(FCB); i++)
	{
		read(fd_fcb, fcb, sizeof(FCB));
		if (strcmp(fcb->nombre_del_archivo, nameArchivo) == 0) {
			return true;
		}
		lseek(fd_fcb, i * sizeof(FCB), SEEK_SET);
	}
	return false;
}

FCB* getArchivo(char * nameArchivo, int* pos) {
	FCB* fcb = malloc(sizeof(FCB));
	lseek(fd_fcb, 0, SEEK_SET);
	for (int i = 0; i < tamanioFile(fd_fcb)/sizeof(FCB); i++)
	{
		read(fd_fcb, fcb, sizeof(FCB));
		if (strcmp(fcb->nombre_del_archivo, nameArchivo) == 0) {
			*pos = i;
			return fcb;
		}
		lseek(fd_fcb, i * sizeof(FCB), SEEK_SET);
	}
	*pos = -1;
	return NULL;
}

int main(int argc, char* argv[]) 
{
	logger = log_create("./../../log.log", "FILESYSTEM", 1, LOG_LEVEL_TRACE);
	log_error(logger, "%s: %i", argv[0], getpid());

	t_config* config = iniciar_config("./../../FileSystem.config");

	char* IP_FILESYSTEM 	= config_get_string_value(config, "IP_FILESYSTEM");
	char* PUERTO_ESCUCHA 	= config_get_string_value(config, "PUERTO_FILESYSTEM");

	char* PATH_SUPERBLOQUE 	= config_get_string_value(config, "PATH_SUPERBLOQUE");
	char* PATH_BITMAP 		= config_get_string_value(config, "PATH_BITMAP");
	char* PATH_BLOQUES 		= config_get_string_value(config, "PATH_BLOQUES");
	char* PATH_FCB 			= config_get_string_value(config, "PATH_FCB");

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");
	
	RETARDO_ACCESO_BLOQUE	= config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

	enviar_mensaje_FILESYSTEM("" , socket_cliente_memory);

    int fd = open(PATH_SUPERBLOQUE, O_RDONLY);
	read(fd, &superBlocked, sizeof(SUPER_BLOCK));
	close(fd);

	fd_fcb 			= open(PATH_FCB, O_RDWR);
	int fd_bloques 	= open(PATH_BLOQUES, O_RDWR | O_CREAT, 0644);
    int fd_bitmap 	= open(PATH_BITMAP, O_RDWR | O_CREAT, 0644);

	ftruncate(fd_bitmap, superBlocked.BLOCK_COUNT/8);
	ftruncate(fd_bloques, superBlocked.BLOCK_COUNT * superBlocked.BLOCK_SIZE);

    void* memoria_bitmap 	= mmap(NULL, superBlocked.BLOCK_COUNT/8, PROT_WRITE | PROT_READ, MAP_SHARED, fd_bitmap, 0);
    void* memoria_bloques 	= mmap(NULL, superBlocked.BLOCK_COUNT * superBlocked.BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);

	bitArrayMap = bitarray_create_with_mode(memoria_bitmap, superBlocked.BLOCK_COUNT/8, MSB_FIRST);

	int socket_servidor = init_socket(IP_FILESYSTEM, PUERTO_ESCUCHA);
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	PCB* pcb;
	op_code opcode;

	while (true)
	{
		pcb = recibirPCB(socket_cliente, &opcode);

		switch (opcode)
		{
			case F_OPEN: 
			{
				opcode = existeArchivo(pcb->registerCPU.R[2]) ? OK : FILE_NO_EXIST;
				if (opcode == OK) 
					log_info(logger, "Abrir Archivo: %s", pcb->registerCPU.R[2]);
			} break;
			case FILE_CREATE:
			{
				FCB fcb;
				strcpy(fcb.nombre_del_archivo, pcb->registerCPU.R[2]);
				fcb.tamanio = 0;
				lseek(fd_fcb, 0, SEEK_END);
				write(fd_fcb, &fcb, sizeof(FCB));
				log_info(logger, "Crear Archivo: %s", pcb->registerCPU.R[2]);
			} break;
			case F_READ:
			{
				log_debug(logger, "F_READ");
				int pos;
				FCB* fcb = getArchivo(pcb->registerCPU.R[2], &pos);
				INFO_MEMORY data;
				int cantInfo = atoi(pcb->registerCPU.R[4]);
				int ptrArchivo = pcb->generico[0];
				for (int i = 0; i < cantInfo; i++) {
					usleep(RETARDO_ACCESO_BLOQUE * 1000);
					int bloque = ceil((float)ptrArchivo / superBlocked.BLOCK_SIZE);
					log_debug(logger, "---------------------------");
					if (bloque == 1) {
						log_debug(logger, "fcb->bloque_directo = %i", fcb->bloque_directo);
						log_debug(logger, "offset = %i", ptrArchivo % superBlocked.BLOCK_SIZE);
						data.info = *((char *)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_directo + (ptrArchivo % superBlocked.BLOCK_SIZE)));
						log_error(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO> - Bloque File System <NUMERO BLOQUE FS>", pcb->registerCPU.R[2]);
					} else if (bloque > 1) {
						log_debug(logger, "fcb->bloque_indirecto = %i", fcb->bloque_indirecto);
						log_debug(logger, "offset = %i", (bloque - 1) * 4 + (ptrArchivo % superBlocked.BLOCK_SIZE));
						data.info = *((char *)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_indirecto + (bloque - 1) * 4 + (ptrArchivo % superBlocked.BLOCK_SIZE)));
					}
					data.ptr = *((int*)(pcb->registerCPU.R[6])) + i;
					memcpy(pcb->generico, &data, sizeof(INFO_MEMORY));
					enviarPCB(pcb, socket_cliente_memory, opcode);
					pcb = recibirPCB(socket_cliente_memory, &opcode);
					ptrArchivo++;
				}
				log_error(logger, "/****************************/");				
				log_info(logger, "Leer Archivo: %s - Puntero: %i - Memoria: %i - Tamaño: %s", pcb->registerCPU.R[2], ptrArchivo, *((int*)(pcb->registerCPU.R[6])), pcb->registerCPU.R[4]);
				opcode = OK;
			} break;
			case F_WRITE:
			{
				/***((int*)(pcb->registerCPU.R[6])) == POS_LEER_MEMORY*/
				/*******pcb->registerCPU.R[7][0] ==== POSICION DONDE GUARDA LA MEMORIA LA INFO*******/
				log_debug(logger, "F_WRITE");
				int pos;
				FCB* fcb = getArchivo(pcb->registerCPU.R[2], &pos);
				for (int i = 0; i < atoi(pcb->registerCPU.R[4]); i++) {
					usleep(RETARDO_ACCESO_BLOQUE * 1000);
					enviarPCB(pcb, socket_cliente_memory, opcode);
					log_debug(logger, "---------------------------------");
					log_debug(logger, "posLeerMemory = %i", *((int*)(pcb->registerCPU.R[6])));
					free(pcb);
					pcb = recibirPCB(socket_cliente_memory, &opcode);
					(*((int*)(pcb->registerCPU.R[6])))++;
					log_error(logger, "info de la memoria = %c", pcb->registerCPU.R[7][0]);
					/********************************************************/
					int cantBloquesAsignados = ceil((float)(fcb->tamanio + 1) / superBlocked.BLOCK_SIZE);
					if (fcb->tamanio % superBlocked.BLOCK_SIZE == 0) {
						if (cantBloquesAsignados == 2) {
							/***********************/
							int z = asignarBit();
							fcb->bloque_indirecto = z;
							log_debug(logger, "Se asigna bloque indirecto= %i", z);
						}
						int z = asignarBit();
						*((uint32_t *)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_indirecto + (cantBloquesAsignados - 2) * 4)) = z;
						log_debug(logger, "Se asigna bloque directo= %i", z);
					}
					log_debug(logger, "cantBloquesAsignados = %i", cantBloquesAsignados);
					if (cantBloquesAsignados == 1) {
						*((char *)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_directo + (fcb->tamanio % superBlocked.BLOCK_SIZE))) = pcb->registerCPU.R[7][0];
						log_debug(logger, "offset = %i", superBlocked.BLOCK_SIZE * fcb->bloque_directo + (fcb->tamanio % superBlocked.BLOCK_SIZE));
					} else if (cantBloquesAsignados > 1) {
						uint32_t offset = *((uint32_t*)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_indirecto + (cantBloquesAsignados - 2) * 4));
						*((char *)(memoria_bloques + offset * superBlocked.BLOCK_SIZE + (fcb->tamanio % superBlocked.BLOCK_SIZE))) = pcb->registerCPU.R[7][0];
						log_debug(logger, "offset relativo = %i", offset);	
						log_debug(logger, "offset absoluto = %i", offset * superBlocked.BLOCK_SIZE + (fcb->tamanio % superBlocked.BLOCK_SIZE));
					}
					fcb->tamanio++;
				}
				
				lseek(fd, pos * sizeof(FCB), SEEK_SET);
				write(fd_fcb, fcb, sizeof(FCB));
				
				log_info(logger, "Escribir Archivo: %s - Puntero: %i - Memoria: %i - Tamaño: %s", pcb->registerCPU.R[2], pcb->generico[0], *((int*)(pcb->registerCPU.R[6])), pcb->registerCPU.R[4]);
				opcode = OK;
			} break;
			case F_TRUNCATE:
			{
				opcode = OK;
				int pos;
				FCB* fcb = getArchivo(pcb->registerCPU.R[2], &pos);
				int tamanioAsignar = atoi(pcb->registerCPU.R[3]) - fcb->tamanio;
				int cantBloquesAsignados = ceil((float)fcb->tamanio / superBlocked.BLOCK_SIZE);
				int tamanioOcupadoEnElUltimoBloque = fcb->tamanio % superBlocked.BLOCK_SIZE;
				int espacioLibreEnElBloque = (superBlocked.BLOCK_SIZE - tamanioOcupadoEnElUltimoBloque) % superBlocked.BLOCK_SIZE;
				int cantBloquesAsignar = ceil((float)(tamanioAsignar - espacioLibreEnElBloque) / superBlocked.BLOCK_SIZE);
				log_error(logger, "-----------------------------");
				log_error(logger, "R[3] = %i tamanio = %i", atoi(pcb->registerCPU.R[3]), fcb->tamanio);
				log_error(logger, "tamanioAsignar = %i", tamanioAsignar);
				log_error(logger, "cantBloquesAsignados = %i", cantBloquesAsignados);
				log_error(logger, "tamanioOcupadoEnElUltimoBloque = %i", tamanioOcupadoEnElUltimoBloque);
				log_error(logger, "espacioLibreEnElBloque = %i", espacioLibreEnElBloque);
				log_error(logger, "cantBloquesAsignar = %i", cantBloquesAsignar);
				if (tamanioAsignar > 0) {
					for (int j = 0; j < cantBloquesAsignar; j++) {
						usleep(RETARDO_ACCESO_BLOQUE * 1000);
						int i = asignarBit();
						log_error(logger, "-----------------------------");
						log_error(logger, "bloque asignado numero = %i", i);
						log_error(logger, "cantBloquesAsignados = %i", cantBloquesAsignados);
						if (cantBloquesAsignados == 0) {
							fcb->bloque_directo = i;
						} else if (cantBloquesAsignados >= 1) {
							if (cantBloquesAsignados == 1) {
								int z = asignarBit();
								fcb->bloque_indirecto = z;
							}
							log_error(logger, "fcb->bloque_indirecto = %i", fcb->bloque_indirecto);
							*((uint32_t *)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_indirecto + (cantBloquesAsignados - 1) * 4)) = i;
							log_error(logger, "Asigno valor a memoria_bloques");
						}
						cantBloquesAsignados++;
					}
				} else if (tamanioAsignar < 0) {
					int cantBloquesSacar = abs(cantBloquesAsignar);
					log_error(logger, "cantBloquesSacar = %i", cantBloquesSacar);
					for (int i = 0; i < cantBloquesSacar; i++) {
						usleep(RETARDO_ACCESO_BLOQUE * 1000);
						log_error(logger, "-----------------------------");
						log_error(logger, "cantBloquesAsignados = %i", cantBloquesAsignados);
						if (cantBloquesAsignados == 1) {
							bitarray_clean_bit(bitArrayMap, fcb->bloque_directo);
							log_error(logger, "se limpia bit %i", fcb->bloque_directo);
						} else if (cantBloquesAsignados > 1) {
							uint32_t bloque = *((uint32_t *)(memoria_bloques + superBlocked.BLOCK_SIZE * fcb->bloque_indirecto + (cantBloquesAsignados - 2) * 4));
							bitarray_clean_bit(bitArrayMap, bloque);
							log_error(logger, "se limpia bit %i", bloque);
							log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 0", bloque);
							if (cantBloquesAsignados == 2) {
								bitarray_clean_bit(bitArrayMap, fcb->bloque_indirecto);
								log_error(logger, "se limpia bit %i", fcb->bloque_indirecto);
								log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 0", fcb->bloque_indirecto);
							}
						}
						cantBloquesAsignados--;
					}
				}
				fcb->tamanio = atoi(pcb->registerCPU.R[3]);
				lseek(fd, pos * sizeof(FCB), SEEK_SET);
				write(fd_fcb, fcb, sizeof(FCB));
				log_info(logger, "Truncar Archivo: %s - Tamaño: %i", fcb->nombre_del_archivo, fcb->tamanio);
			} break;
			default: break;
		}

		enviarPCB(pcb, socket_cliente, opcode);
	}

	return EXIT_SUCCESS;
}
