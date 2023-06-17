#include <stdio.h>
#include <sys/socket.h>
#include "netdb.h"
#include "string.h"
#include "my_commons.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <sys/types.h>
#include <unistd.h>

int main() 
{
	t_config* config = iniciar_config();

	char* ip 		 = config_get_string_value(config, "IP_KERNEL");
	char* puerto 	 = config_get_string_value(config, "PUERTO_KERNEL");
	
	int socket_cliente = crear_conexion(ip, puerto);
	
	PCB pcb;
	pcb.PID = getpid();
	pcb.PC = 0;

	FILE* fd = fopen("./../../assembler.s","r");

	char buffer[256]; // Buffer para almacenar la línea leída

	Node* list = NULL;

	int len = 0;
	
	while (fgets(buffer, sizeof(buffer), fd) != NULL) {
    
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

	fclose(fd); // Cierra el archivo
	enviarPCB(&pcb, socket_cliente, MENSAJE);

	return EXIT_SUCCESS;
}
