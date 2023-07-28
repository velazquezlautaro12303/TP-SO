#include <stdio.h>
#include "my_commons.h"
#include <pthread.h>
#include <math.h>
#include <semaphore.h>

sem_t semaphore_MEMORY_1, semaphore_MEMORY_2;
op_code opcode;
int tam_max_segmento;

PCB* pcb;
t_log* logger;
int RETARDO_INSTRUCCION;

char RAX[16];
char RBX[16];
char RCX[16];
char RDX[16];
char R[CANT_REG_PROPOSITO_GENERAL][16];
char* EAX;
char* EBX;
char* ECX;
char* EDX;
char* AX;
char* BX;
char* CX;
char* DX;

#define GET_OPCODE(cadena, cadena2, opcode) \
	if (strcmp(cadena, cadena2) == 0) { 	\
		return opcode;						\
	}

#define COPY_CLEAN(cadena_pcb, cadena)		\
	if (cadena != NULL) {					\
		strncpy(cadena_pcb, cadena, 16);	\
		memcpy(cadena, "\0", 16);			\
	}

#define WRITE_REG(REG, VALUE)			\
	set_reg(REG, "RAX", RAX, VALUE);	\
	set_reg(REG, "RBX", RBX, VALUE);	\
	set_reg(REG, "RCX", RCX, VALUE);	\
	set_reg(REG, "RDX", RDX, VALUE);	\
	set_reg(REG, "EAX", EAX, VALUE);	\
	set_reg(REG, "EBX", EBX, VALUE);	\
	set_reg(REG, "ECX", ECX, VALUE);	\
	set_reg(REG, "EDX", EDX, VALUE);	\
	set_reg(REG, "AX", AX, VALUE);		\
	set_reg(REG, "BX", BX, VALUE);		\
	set_reg(REG, "CX", CX, VALUE);		\
	set_reg(REG, "DX", DX, VALUE);

#define READ_REG(REG, VALUE)			\
	set_reg(REG, "RAX", VALUE, RAX);	\
	set_reg(REG, "RBX", VALUE, RBX);	\
	set_reg(REG, "RCX", VALUE, RCX);	\
	set_reg(REG, "RDX", VALUE, RDX);	\
	set_reg(REG, "EAX", VALUE, EAX);	\
	set_reg(REG, "EBX", VALUE, EBX);	\
	set_reg(REG, "ECX", VALUE, ECX);	\
	set_reg(REG, "EDX", VALUE, EDX);	\
	set_reg(REG, "AX", VALUE, AX);		\
	set_reg(REG, "BX", VALUE, BX);		\
	set_reg(REG, "CX", VALUE, CX);		\
	set_reg(REG, "DX", VALUE, DX);

void set_reg(char* cadena, char* cadena2, char* dest, char* value)
{
	if (strcmp(cadena, cadena2) == 0) 
		strncpy(dest, value, size_reg(cadena));
}

static void copiarRegCPUaRegPCB(PCB* pcb)
{
	memcpy(pcb->registerCPU.RAX, RAX, 16);
	memcpy(pcb->registerCPU.RBX, RBX, 16);
	memcpy(pcb->registerCPU.RCX, RCX, 16);
	memcpy(pcb->registerCPU.RDX, RDX, 16);
	for (int i = 0; i < CANT_REG_PROPOSITO_GENERAL; i++) {
		COPY_CLEAN(pcb->registerCPU.R[i], R[i]);
	}
}

static void copiarRegDePCBaRegCPU(PCB* pcb)
{
	memcpy(RAX, pcb->registerCPU.RAX, 16);
	memcpy(RBX, pcb->registerCPU.RBX, 16);
	memcpy(RCX, pcb->registerCPU.RCX, 16);
	memcpy(RDX, pcb->registerCPU.RDX, 16);
	for (int i = 0; i < CANT_REG_PROPOSITO_GENERAL; i++) {
		COPY_CLEAN(R[i], pcb->registerCPU.R[i]);
	}
}

static char* obtenerCadenaHastaEspacio(char** cadena, int* endString) {
    int longitud = 0;
	while ((*cadena)[longitud] != ' ') {
		if ((*cadena)[longitud] == '\0') break;
        longitud++;
    }
	*endString = (*cadena)[longitud] == '\0' ? 1 : 0;
    char* resultado = malloc((longitud + 1) * sizeof(char));
    strncpy(resultado, *cadena, longitud + 1);
	*cadena = &((*cadena)[longitud + 1]);
    resultado[longitud] = '\0';

    return resultado;
}

static op_code obtenerOpcode() 
{
	GET_OPCODE(R[1], "SET", SET);
	GET_OPCODE(R[1], "MOV_IN", MOV_IN);
	GET_OPCODE(R[1], "MOV_OUT", MOV_OUT);
	GET_OPCODE(R[1], "I/O", I_O);
	GET_OPCODE(R[1], "F_OPEN", F_OPEN);
	GET_OPCODE(R[1], "F_CLOSE", F_CLOSE);
	GET_OPCODE(R[1], "F_SEEK", F_SEEK);
	GET_OPCODE(R[1], "F_READ", F_READ);
	GET_OPCODE(R[1], "F_WRITE", F_WRITE);
	GET_OPCODE(R[1], "F_TRUNCATE", F_TRUNCATE);
	GET_OPCODE(R[1], "WAIT", WAIT);
	GET_OPCODE(R[1], "SIGNAL", SIGNAL);
	GET_OPCODE(R[1], "CREATE_SEGMENT", CREATE_SEGMENT);
	GET_OPCODE(R[1], "DELETE_SEGMENT", DELETE_SEGMENT);
	GET_OPCODE(R[1], "YIELD", YIELD);
	GET_OPCODE(R[1], "EXIT", _EXIT);
	return EXIT;
}

static op_code mmu(PCB* pcb, int dir_logica, int* num_segmento, int* desplazamiento_segmento)
{
	*num_segmento = floor(dir_logica / tam_max_segmento);
	*desplazamiento_segmento = dir_logica % tam_max_segmento;
	if (pcb->tablaSegmentos[*num_segmento].lenSegmentoDatos < *desplazamiento_segmento) {
		return SEGMENT_FAULT;
	}
	return MENSAJE;
}

void fetch()
{
	char * pcbInstruccion = getInstruction(pcb->listInstrucciones.instrucciones, (pcb->PC)++);
	strcpy(R[0], pcbInstruccion);
}

void decode() 
{
	int condition = 0;
	char *ptr = (char *)malloc(strlen(R[0]) + 1);
	strcpy(ptr, R[0]);
	char *cadena = ptr;
	int REG_N = 0;
	do {
		strncpy(R[++REG_N], obtenerCadenaHastaEspacio(&cadena, &condition), 16);
	} while(!condition);
	free(ptr);
	opcode = obtenerOpcode();
}

bool execute()
{
	log_info(logger, "PID: %i - Ejecutando: %s - %s %s %s", pcb->PID, R[1], R[2], R[3], R[4]);
	switch (opcode)
	{
		case SET:
			{
				usleep(RETARDO_INSTRUCCION * 1000);
				WRITE_REG(R[2], R[3]);
			} break;

		case MOV_IN:
			{
				int num_segmento, desplazamiento, dir_logica = atoi(R[3]);
				opcode = mmu(pcb, dir_logica, &num_segmento, &desplazamiento);
				memcpy(R[0], &num_segmento, sizeof(int));
				memcpy(R[1], &desplazamiento, sizeof(int));
				if (opcode == SEGMENT_FAULT) {
					return true;
				} else {
					opcode = MOV_IN;
					copiarRegCPUaRegPCB(pcb);
					sem_post(&semaphore_MEMORY_1);
					sem_wait(&semaphore_MEMORY_2);
					WRITE_REG(pcb->registerCPU.R[2], pcb->registerCPU.R[4]);
				}
			} break;

		case MOV_OUT: 
			{
				int num_segmento, desplazamiento, dir_logica = atoi(R[2]);
				opcode = mmu(pcb, dir_logica, &num_segmento, &desplazamiento);
				memcpy(R[0], &num_segmento, sizeof(int));
				memcpy(R[1], &desplazamiento, sizeof(int));
				if (opcode == SEGMENT_FAULT) {
					return true;
				} else {
					opcode = MOV_OUT;
					READ_REG(R[3], R[4]);
					copiarRegCPUaRegPCB(pcb);
					sem_post(&semaphore_MEMORY_1);
					sem_wait(&semaphore_MEMORY_2);
				}
			} break;

		case I_O:
			{
				opcode = BLOCKED;
				return true;
			} break;

		case F_WRITE:
		case F_READ:
			{
				int num_segmento, desplazamiento, dir_logica = atoi(R[3]);
				int opcode_aux = mmu(pcb, dir_logica, &num_segmento, &desplazamiento);
				memcpy(R[6], &num_segmento, sizeof(int));
				memcpy(R[7], &desplazamiento, sizeof(int));
				if (opcode_aux == SEGMENT_FAULT) {
					opcode = opcode_aux;
				} else {
					opcode_aux = opcode;
					opcode = GET_DIR_FISICA;
					copiarRegCPUaRegPCB(pcb);
					sem_post(&semaphore_MEMORY_1);
					sem_wait(&semaphore_MEMORY_2);
					copiarRegDePCBaRegCPU(pcb);
					opcode = opcode_aux;
					memcpy(R[5], &opcode, sizeof(op_code));
				}
				return true;
			} break;

		case F_TRUNCATE:
			{
				memcpy(R[5], &opcode, sizeof(op_code));
				return true;
			} break;
			
		case F_OPEN:
		case F_CLOSE:
		case F_SEEK:
		case WAIT:
		case SIGNAL:
		case CREATE_SEGMENT:
		case DELETE_SEGMENT:
		case YIELD:
		case _EXIT:
			{
				return true;
			} break;

		default: break;
	}
	return false;
}

void* conectarse_memory(void* ptr)
{
	t_config* config = iniciar_config("./../../CPU.config");

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	
	enviar_mensaje_CPU("", socket_cliente_memory);

	while (true)
	{
		sem_wait(&semaphore_MEMORY_1);

		enviarPCB(pcb, socket_cliente_memory, opcode);
		free(pcb);
		pcb = recibirPCB(socket_cliente_memory, &opcode);
		sem_post(&semaphore_MEMORY_2);
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	logger = log_create("./../../log.log", "CPU", 1, LOG_LEVEL_TRACE);
	log_error(logger, "%s: %i", argv[0], getpid());

	t_config* config = iniciar_config("./../../CPU.config");

	sem_init(&semaphore_MEMORY_1, 0, 0);
	sem_init(&semaphore_MEMORY_2, 0, 0);

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread);

	char* IP_CPU 		= config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU 	= config_get_string_value(config, "PUERTO_CPU");
	tam_max_segmento 	= config_get_int_value(config, "TAM_MAX_SEGMENTO");
	RETARDO_INSTRUCCION = config_get_int_value(config, "RETARDO_INSTRUCCION");

	EAX = RAX + 8;
	EBX = RAX + 8;
	ECX = RAX + 8;
	EDX = RAX + 8;

	AX = RAX + 12;
	BX = RAX + 12;
	CX = RAX + 12;
	DX = RAX + 12;

	memcpy(RAX, "\0", 16);
	memcpy(RBX, "\0", 16);
	memcpy(RCX, "\0", 16);
	memcpy(RDX, "\0", 16);

	int socket_servidor = init_socket(IP_CPU, PUERTO_CPU);
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	while (true) 
	{
		pcb = recibirPCB(socket_cliente, &opcode);
		copiarRegDePCBaRegCPU(pcb);
		memcpy(pcb->registerCPU.R[0], "\0", 16*CANT_REG_PROPOSITO_GENERAL);

		do {
			for (int i = 0; i < CANT_REG_PROPOSITO_GENERAL; i++)
				memcpy(R[i], "\0", 16);
			fetch();
			decode();
			if (execute()) break;
		} while (pcb->PC < pcb->listInstrucciones.cant);

		copiarRegCPUaRegPCB(pcb);

		enviarPCB(pcb, socket_cliente, opcode);
		free(pcb);
	}

	return EXIT_SUCCESS;
}
