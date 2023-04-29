#ifndef UTILS_H_
#define UTILS_H_

#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<commons/log.h>
#include<stdio.h>
#include<stdlib.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include <commons/config.h>

#define LOCALHOST "127.0.0.1"

typedef enum
{
	CPU					= 0,
	KERNEL 				= 1,
	FILESYSTEM			= 2,
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

void enviar_mensaje_CPU(char* mensaje, int socket_cliente);
void enviar_mensaje_FILESYSTEM(char* mensaje, int socket_cliente);
void enviar_mensaje_KERNEL(char* mensaje, int socket_cliente);
t_config* iniciar_config();
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
t_paquete* crear_super_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

extern t_log* logger;

void* recibir_buffer(int*, int);

int init_socket(char*, char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

#endif /* UTILS_H_ */