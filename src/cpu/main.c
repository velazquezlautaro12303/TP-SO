#include <stdio.h>
#include "../our_commons/utils.h"
#include <pthread.h>

// int* traducirDireccionesLogicaFisica(int * direccion)
// {

// }

static char* obtenerCadenaHastaEspacio(char* cadena, int* endString) {
    int longitud = 0;
    while (cadena[longitud] != ' ' || cadena[longitud] != '\0') {
        longitud++;
    }

	*endString = cadena[longitud] == '\0' ? 1 : 0;
    char* resultado = malloc((longitud + 1) * sizeof(char));
    strncpy(resultado, cadena, longitud);
    resultado[longitud] = '\0';

    return resultado;
}

char* fetch(PCB* pcb)
{
	return pcb->instrucciones[pcb->PC++];
}

void decode(char * REG_IR) 
{
	int condition = 0;
	do {
		puts(obtenerCadenaHastaEspacio(REG_IR, &condition));
	} while(condition != 0);
}

void execute(OPCODE opcode)
{
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
			/* code */
			break;

		case F_OPEN:
			/* code */
			break;

		case F_CLOSE:
			/* code */
			break;

		case F_SEEK:
			/* code */
			break;

		case F_READ:
			/* code */
			break;

		case F_WRITE:
			/* code */
			break;

		case F_TRUNCATE:
			/* code */
			break;

		case WAIT:
			/* code */
			break;

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
			/* code */
			break;
	}
}

void* atender_cliente(void* socket_cliente)
{
	int socket_console = *((int *)socket_cliente);
	int cod_op = recibir_operacion(socket_console);
	switch (cod_op)
	{
		case MENSAJE:
			recibir_structura(socket_console);
			break;
	}
	PCB pcb;
	OPCODE opcode = SET;
	/*PIPELINE*/
	char * REG_IR = fetch(&pcb);
	decode(REG_IR);
	execute(opcode);
}

void* conectarse_memory(void* ptr)
{
	t_config* config = iniciar_config();

	char* IP_MEMORIA 		= config_get_string_value(config, "IP_MEMORIA");
	char* PUERTO_MEMORIA 	= config_get_string_value(config, "PUERTO_MEMORIA");

	int socket_cliente_memory = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje_CPU("asd", socket_cliente_memory);
}

int main()
{
	logger = log_create("./../log.log", "CPU", 1, LOG_LEVEL_DEBUG);

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) conectarse_memory, NULL);
	pthread_detach(thread);

	t_config* config = iniciar_config();

	char* IP_CPU 		= config_get_string_value(config, "IP_CPU");
	char* PUERTO_CPU 	= config_get_string_value(config, "PUERTO_CPU");

	int socket_servidor = init_socket(IP_CPU, PUERTO_CPU);

	while(1)
	{
		pthread_t thread;
		int *socket_cliente = malloc(sizeof(int));
		*socket_cliente = accept(socket_servidor, NULL, NULL);
		pthread_create(&thread, NULL, (void*) atender_cliente, socket_cliente);
		pthread_detach(thread);
	}

	return EXIT_SUCCESS;
}
