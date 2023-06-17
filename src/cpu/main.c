#include <stdio.h>
#include "my_commons.h"
#include <pthread.h>

#define GET_OPCODE(cadena, cadena2, opcode) \
	if (strcmp(cadena, cadena2) == 0) { 	\
		return opcode;						\
	}

char* REG_IR[5];
t_log* logger;

// int* traducirDireccionesLogicaFisica(int * direccion)
// {

// }

// op_code mmu() {
// 	return ;
// }

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

static OPCODE obtenerOpcode() 
{
	GET_OPCODE(REG_IR[1], "SET", SET);
	GET_OPCODE(REG_IR[1], "MOV_IN", MOV_IN);
	GET_OPCODE(REG_IR[1], "MOV_OUT", MOV_OUT);
	GET_OPCODE(REG_IR[1], "I/O", I_O);
	GET_OPCODE(REG_IR[1], "F_OPEN", F_OPEN);
	GET_OPCODE(REG_IR[1], "F_CLOSE", F_CLOSE);
	GET_OPCODE(REG_IR[1], "F_SEEK", F_SEEK);
	GET_OPCODE(REG_IR[1], "F_READ", F_READ);
	GET_OPCODE(REG_IR[1], "F_WRITE", F_WRITE);
	GET_OPCODE(REG_IR[1], "F_TRUNCATE", F_TRUNCATE);
	GET_OPCODE(REG_IR[1], "WAIT", WAIT);
	GET_OPCODE(REG_IR[1], "SIGNAL", SIGNAL);
	GET_OPCODE(REG_IR[1], "CREATE_SEGMENT", CREATE_SEGMENT);
	GET_OPCODE(REG_IR[1], "DELETE_SEGMENT", DELETE_SEGMENT);
	GET_OPCODE(REG_IR[1], "YIELD", YIELD);
	GET_OPCODE(REG_IR[1], "EXIT", C_EXIT);
	return C_EXIT;
}

void fetch(PCB* pcb)
{
	char * pcbInstruccion = getInstruction(pcb->listInstrucciones.instrucciones, (pcb->PC)++);
	REG_IR[0] = (char *)malloc(strlen(pcbInstruccion) + 1);
	strcpy(REG_IR[0], pcbInstruccion);
}

OPCODE decode() 
{
	int condition = 0;
	char *cadena = (char *)malloc(strlen(REG_IR[0]) + 1);
	strcpy(cadena, REG_IR[0]);
	char *ptr = cadena;
	int REG_N = 0;
	do {
		REG_IR[++REG_N] = obtenerCadenaHastaEspacio(&REG_IR[0], &condition);
	} while(!condition);
	free(ptr);
	return obtenerOpcode();
}

bool execute(OPCODE opcode, PCB* pcb, op_code *op_cod)
{
	log_info(logger, "PID: %i - Ejecutando: %s\n", pcb->PID, REG_IR[1]);
	switch (opcode)
	{
		case SET:
			/* code */
			break;

		case MOV_IN:
			/* code */
			break;

		case MOV_OUT:
			/* code */
			break;

		case I_O:
			{
				*op_cod = BLOCKED;
				return true;
			} break;

		case F_OPEN:
			{
				*op_cod = FILESYSTEM;
				return true;
			} break;

		case F_CLOSE:
			{
				*op_cod = FILESYSTEM;
				return true;
			} break;

		case F_SEEK:
			{
				*op_cod = FILESYSTEM;
				return true;
			} break;

		case F_READ:
			{
				*op_cod = FILESYSTEM;
				return true;
			} break;

		case F_WRITE:
			{
				*op_cod = FILESYSTEM;
				return true;
			} break;

		case F_TRUNCATE:
			{
				*op_cod = FILESYSTEM;
				return true;
			} break;

		case WAIT:
			{
				*op_cod = BLOCKED;
				return true;
			} break;

		case SIGNAL:
			/* code */
			break;

		case CREATE_SEGMENT:
			/* code */
			break;

		case DELETE_SEGMENT:
			/* code */
			break;

		case YIELD:
			/* code */
			break;

		case C_EXIT:
			{
				*op_cod = _EXIT;
				return true;
			} break;
	}
	return false;
}

void* conectarse_memory(void* ptr)
{
	t_config* config = iniciar_config();

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

	enviar_mensaje_CPU("", socket_cliente_memory);

	// enviar_structura(, socket_cliente_memory, , CPU);

	return NULL;
}

int main()
{
	logger = log_create("./../../log.log", "CPU", 0, LOG_LEVEL_INFO);
	t_config* config = iniciar_config();

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread);

	char* IP_CPU 		= config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU 	= config_get_string_value(config, "PUERTO_CPU");

	int socket_servidor = init_socket(IP_CPU, PUERTO_CPU);
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	op_code cod_op;
	
	while (true) 
	{
		PCB* pcb = recibirPCB(socket_cliente, &cod_op);

		do {
			fetch(pcb);
			OPCODE opcode = decode();
			if (execute(opcode, pcb, &cod_op)) break;
		} while (pcb->PC < pcb->listInstrucciones.cant);

		enviarPCB(pcb, socket_cliente, cod_op);
		free(pcb);
	}

	return EXIT_SUCCESS;
}
