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
	PAQUETE,
	BLOCKED,
	_EXIT,
	INIT_PROCESS
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

void* recibir_buffer(int*, int);

int init_socket(char*, char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

typedef enum OPCODE
{
	SET,
	MOV_IN,
	MOV_OUT,
	I_O,
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
	WAIT,
	SIGNAL,
	CREATE_SEGMENT,
	DELETE_SEGMENT,
	YIELD,
	C_EXIT
} OPCODE;


typedef struct TABLE_SEGMENTS
{
	/*FALTA AGREGAR ALGO Q NO SE QUE ES?*/
	void* direccionBase;
	uint32_t lenSegmentoDatos;
} TABLE_SEGMENTS;

typedef struct REGISTERS_CPU
{
	uint32_t R[13];
} REGISTERS_CPU;

typedef struct Node { 
    //creo un nodo el cual servira de listado para las instrucciones del archivo
    char* instrucciones;
    struct Node* next;
} Node;

typedef struct LIST_INSTRUCCIONES
{
	int cant;
	Node* instrucciones;
} LIST_INSTRUCCIONES;

typedef struct PCB
{
	pid_t PID;
	LIST_INSTRUCCIONES listInstrucciones;	//Cambiar por ptr
	int PC;
	REGISTERS_CPU registerCPU;
	TABLE_SEGMENTS* tablaSegmentos;
	int estimadoProxRafaga;
	int tiempoLlegadaReady;
	int tablaArchivos;	//Cambiar por ptr
} PCB;

typedef struct tnodo
{
   PCB* info;
   struct tnodo* sig;
} nodo;

typedef nodo* ptrNodo;

typedef enum STATES {
	NEW, READY, EXEC, EXIT, BLOCK
} STATES;

void push(ptrNodo* pila, PCB* info);
PCB* pop(ptrNodo* pila);

void enviar_structura(void* mensaje, int socket_cliente, int size, op_code cod_operation);
void* recibir_structura(int socket_cliente);

void insert(Node** head, char* instrucciones);
char* get(Node** head);
int list_length(Node* head);
PCB* recibirPCB(int socket_cliente, op_code* cod_operation);
void enviarPCB(PCB* pcb, int socket_cliente, op_code cod_operation);
char* getInstruction(Node* head, int pos);

#endif /* UTILS_H_ */
