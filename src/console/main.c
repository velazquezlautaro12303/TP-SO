#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "my_commons.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
	t_config* config = iniciar_config("./../../Console.config");

	char* ip 		 = config_get_string_value(config, "IP_KERNEL");
	char* puerto 	 = config_get_string_value(config, "PUERTO_KERNEL");
	
	int socket_cliente = crear_conexion(ip, puerto);
	
	PCB pcb;
	pcb.PID = getpid();
	printf("PID = %i\n", pcb.PID);
	pcb.PC = 0;
	pcb.tablaSegmentos = NULL;
	pcb.CANT_SEGMENTOS = 0;
	memcpy(pcb.registerCPU.R, "\0", 16*CANT_REG_PROPOSITO_GENERAL);
	memcpy(pcb.registerCPU.RAX, "\0", 16);
	memcpy(pcb.registerCPU.RBX, "\0", 16);
	memcpy(pcb.registerCPU.RCX, "\0", 16);
	memcpy(pcb.registerCPU.RDX, "\0", 16);

	FILE* fd = fopen(argv[1],"r");

	char buffer[256]; // Buffer para almacenar la línea leída

	Node* list = NULL;
	
	int len = 0;
	
	while (fgets(buffer, sizeof(buffer), fd) != NULL) 
	{
		len = strlen(buffer);
		buffer[len - 1] = '\0';

		insert(&list, buffer);

        // Limpia el buffer para la próxima línea
        for (int i = 0; i < sizeof(buffer); i++) {
            buffer[i] = '\0';
        }
    }

	pcb.listInstrucciones.cant = list_length(list);
	pcb.listInstrucciones.instrucciones = list;

	pcb.tablaArchivos.cant = 0;
	pcb.tablaArchivos.fcb = NULL;
	pcb.generico[0] = 0;
	pcb.generico[1] = 0;

	fclose(fd); // Cierra el archivo

	enviarPCB(&pcb, socket_cliente, MENSAJE);

	return EXIT_SUCCESS;
}
