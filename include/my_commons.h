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
#include<commons/config.h>
#include<commons/temporal.h>

#define LOCALHOST "127.0.0.1"

#define CANT_REG_PROPOSITO_GENERAL 8

typedef struct
{
	uint32_t ptr;
	char info;
} INFO_MEMORY;

typedef enum
{
	CPU					= 0,
	KERNEL 				= 1,
	FILESYSTEM			= 2,
	MENSAJE,
	PAQUETE,
	BLOCKED,
	_EXIT,
	INIT_PROCESS,
	SET,
	MOV_IN,
	MOV_OUT,
	GET_DIR_FISICA,
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
	SEGMENT_FAULT,
	FILE_NO_EXIST,
	FILE_CREATE,
	OK
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

typedef struct 
{
	char nombre_del_archivo[32];
	uint32_t ptr_Archivo;
} INODO_PROCESO;

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
char* recibir_mensaje(int socket_cliente, int* size);
int recibir_operacion(int);

typedef struct
{
	int direccionBase;
	int lenSegmentoDatos;
	int pid;
	int idSegmento;
} TABLE_SEGMENTS;

typedef struct REGISTERS_CPU
{
	char RAX[16];
	char RBX[16];
	char RCX[16];
	char RDX[16];
	char R[CANT_REG_PROPOSITO_GENERAL][16];
} REGISTERS_CPU;

typedef struct Node_FCB { 
    INODO_PROCESO inodo;
    struct Node_FCB* next;
} Node_FCB;

typedef struct LIST_FCB
{
	int cant;
	Node_FCB* fcb;
} LIST_FCB;

typedef struct Node { 
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
	time_t estimadoProxRafaga;
	time_t tiempoLlegadaReady;
	LIST_FCB tablaArchivos;	//Cambiar por ptr
	int CANT_SEGMENTOS;
	int generico[2];
	t_temporal* timer;
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

typedef struct
{
	char nombre_del_archivo[32];
	int cantProcesos;
	ptrNodo pilaFILE_FIFO;
} INODO_GLOBAL;

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

int size_reg(char *cadena);

void insert_INODO(Node_FCB** head, INODO_PROCESO* inodo);
INODO_PROCESO* get_INODO(Node_FCB** head);

#endif /* UTILS_H_ */
