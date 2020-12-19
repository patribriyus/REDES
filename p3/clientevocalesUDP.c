/* $Revision: 2f87fa110238e0fdaa9cd874c0195b05a191e79c $ */

//importación de funciones, constantes, etc.
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>


//definición de constantes
#define MAX_BUFF_SIZE 1000 //establecemos este valor máximo para el buffer


// cabeceras de funciones
struct addrinfo* obtener_struct_direccion(char *nodo, char *servicio, char f_verbose);
void printsockaddr(struct sockaddr_storage * saddr);
int initsocket(struct addrinfo *servinfo, char f_verbose);


/***************************** MAIN ******************************/
int main(int argc, char * argv[]) {

	//declaración de variables propias del programa principal (locales a main)
	char f_verbose=1; //flag, 1: imprimir por pantalla información extra
	const char fin = 4; // carácter ASCII end of transmission (EOT) para indicar fin de transmisión
	struct addrinfo * servinfo; // puntero a estructura de dirección destino
	int sock; // descriptor del socket
	char msg[MAX_BUFF_SIZE]; // buffer donde almacenar lo leído y enviarlo
	ssize_t len, // tamaño de lo leído por la entrada estándar (size_t con signo)
			sentbytes; // tamaño de lo enviado (size_t con signo)
	uint32_t num; // variable donde anotar el número de vocales

	//verificación del número de parámetros:
	if (argc != 3) {
		printf("Número de parámetros incorrecto \n");
		printf("Uso : %s servidor puerto/servicio\n", argv[0]);
		exit(1); //sale del programa indicando salida incorrecta (1)
	}

	// obtiene estructura de direccion
	servinfo=obtener_struct_direccion(argv[1], argv[2], f_verbose);

	// crea un extremo de la comunicación con la primera de las direcciones de servinfo e inicia la conexión con el servidor. Devuelve el descriptor del socket
	sock = initsocket(servinfo, f_verbose);

	// cuando ya no se necesite, hay que liberar la memoria dinámica usada para la dirección
	freeaddrinfo(servinfo);
	servinfo=NULL; // como ya no tenemos la memoria, dejamos de apuntarla para evitar acceder a ella por error

	// bucle que lee texto del teclado y lo envía al servidor
	printf("\nTeclea el texto a enviar y pulsa <Enter>, o termina con <Ctrl+d>\n");
	while ((len = read(0, msg, MAX_BUFF_SIZE)) > 0) { 
		// read lee del teclado hasta que se pulsa INTRO, almacena lo leído en msg y devuelve la longitud en bytes de los datos leídos
		if (f_verbose) 
			printf("  Leídos %zd bytes\n",len);

		if ((sentbytes=sendto(sock,msg,len,0,servinfo->ai_addr,servinfo->ai_addrlen)) < 0) { //envía datos al socket
			perror("Error de escritura en el socket");
			exit(1);
		} else { if (f_verbose) 
			printf("  Enviados correctamente %zd bytes \n",sentbytes);
		}
		// en caso de que el socket sea cerrado por el servidor, al llamar a send se genera una señal SIGPIPE, que como en este código no se captura, hace que finalice el programa SIN mensaje de error
		// Las señales se estudian en la asignatura Sistemas Operativos

		printf("Teclea el texto a enviar y pulsa <Enter>, o termina con <Ctrl+d>\n");
	}

	//se envía una marca de finalización:
	if (sendto(sock,&fin,1,0,servinfo -> ai_addr,servinfo -> ai_addrlen) < 0) {
		perror("Error de escritura en el socket");
		exit(1);
	}
	if (f_verbose) {
	  printf("Enviada correctamente la marca de finalización.\nEsperando respuesta del servidor...");
	  fflush(stdout);
	}

	//recibe del servidor el número de vocales recibidas:
	if (recvfrom(sock,&num,4,0,servinfo -> ai_addr,&servinfo -> ai_addrlen) < 0) {
		perror("Error de lectura en el socket");
		exit(1);
	}
	printf(" hecho\nEl texto enviado contenía en total %d vocales\n", num);
	//convierte el entero largo sin signo desde el orden de bytes de la red al del host

	close(sock); //cierra la conexión del socket:
	if(f_verbose)
	  printf("Socket cerrado\n");

	exit(0); //sale del programa indicando salida correcta (0)

}


/************************** initsocket ***************************/
//función que crea la conexión y se conecta al servidor
int initsocket(struct addrinfo *servinfo, char f_verbose){
	int sock;

	printf("\nSe usará ÚNICAMENTE la primera dirección de la estructura\n");

	//crea un extremo de la comunicación y devuelve un descriptor
	if (f_verbose) { printf("Creando el socket (socket)... "); fflush(stdout); }
	sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
	if (sock < 0) {
		perror("Error en la llamada socket: No se pudo crear el socket");
		/*muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
		exit(1);
	}
	if (f_verbose) { printf("hecho\n"); }

	return sock;
}

/******************** obtener_struct_direccion *********************/
/**
 * Función que, en base a ciertos parámetros, devuelve una estructura de direcciones rellenada con al menos una dirección que cumpla los parámetros especificados. El último parámetro sirve para que muestre o no los printfs
 */
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose){
	struct addrinfo hints, //estructura hints para especificar la solicitud
					*servinfo; // puntero al addrinfo devuelto
	int status; // indica la finalización correcta o no de la llamada getaddrinfo
	int numdir=1; // contador de estructuras de direcciones en la lista de direcciones de servinfo
	struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo

	// genera una estructura de dirección con especificaciones de la solicitud
	if (f_verbose) printf("1 - Especificando detalles de la estructura de direcciones a solicitar... \n");
	// sobreescribimos con ceros la estructura para borrar cualquier dato que pueda malinterpretarse
	memset(&hints, 0, sizeof hints); 
   
	if (f_verbose) { printf("\tFamilia de direcciones/protocolos: "); fflush(stdout);}
	hints.ai_family= AF_UNSPEC; // sin especificar: AF_UNSPEC; IPv4: AF_INET; IPv6: AF_INET6; etc.
	if (f_verbose) { 
		switch (hints.ai_family) {
			case AF_UNSPEC: printf("IPv4 e IPv6\n"); break;
			case AF_INET: printf("IPv4)\n"); break;
			case AF_INET6: printf("IPv6)\n"); break;
			default: printf("No IP (%d)\n",hints.ai_family); break;
		}
	}
   
	if (f_verbose) { printf("\tTipo de comunicación: "); fflush(stdout);}
	hints.ai_socktype= SOCK_STREAM; // especificar tipo de socket 
	if (f_verbose) { 
		switch (hints.ai_socktype) {
			case SOCK_STREAM: printf("flujo (TCP)\n"); break;
			case SOCK_DGRAM: printf("datagrama (UDP)\n"); break;
			default: printf("no convencional (%d)\n",hints.ai_socktype); break;
		}
	}
   
	// pone flags específicos dependiendo de si queremos la dirección como cliente o como servidor
	if (dir_servidor!=NULL) {
		// si hemos especificado dir_servidor, es que somos el cliente y vamos a conectarnos con dir_servidor
		if (f_verbose) printf("\tNombre/dirección del equipo: %s\n",dir_servidor); 
	} else {
		// si no hemos especificado, es que vamos a ser el servidor
		if (f_verbose) printf("\tNombre/dirección del equipo: ninguno (seremos el servidor)\n"); 
		hints.ai_flags= AI_PASSIVE; // poner flag para que la IP se rellene con lo necesario para hacer bind
	}
	if (f_verbose) printf("\tServicio/puerto: %s\n",servicio);
	

	// llamada a getaddrinfo para obtener la estructura de direcciones solicitada
	// getaddrinfo pide memoria dinámica al SO, la rellena con la estructura de direcciones, y escribe en servinfo la dirección donde se encuentra dicha estructura
	// la memoria *dinámica* creada dentro de una función NO se destruye al salir de ella. Para liberar esta memoria, usar freeaddrinfo()
	if (f_verbose) { printf("2 - Solicitando la estructura de direcciones con getaddrinfo()... "); fflush(stdout);}
	status = getaddrinfo(dir_servidor,servicio,&hints,&servinfo);
	if (status!=0) {
		fprintf(stderr,"Error en la llamada getaddrinfo: %s\n",gai_strerror(status));
		exit(1);
	} 
	if (f_verbose) { printf("hecho\n"); }


	// imprime la estructura de direcciones devuelta por getaddrinfo()
	if (f_verbose) {
		printf("3 - Analizando estructura de direcciones devuelta... \n");
		direccion=servinfo;
		while (direccion!=NULL) { // bucle que recorre la lista de direcciones
			printf("    Dirección %d:\n",numdir);
			printsockaddr((struct sockaddr_storage*)direccion->ai_addr);
			// "avanzamos" direccion a la siguiente estructura de direccion
			direccion=direccion->ai_next;
			numdir++;
		}
	}

	// devuelve la estructura de direcciones devuelta por getaddrinfo()
	return servinfo;
}


/***************************** printsockaddr ******************************/
/**
 * Imprime una estructura sockaddr_in o sockaddr_in6 almacenada en sockaddr_storage
 */
void printsockaddr(struct sockaddr_storage * saddr) {
	struct sockaddr_in  *saddr_ipv4; // puntero a estructura de dirección IPv4
	// el compilador interpretará lo apuntado como estructura de dirección IPv4
	struct sockaddr_in6 *saddr_ipv6; // puntero a estructura de dirección IPv6
	// el compilador interpretará lo apuntado como estructura de dirección IPv6
	void *addr; // puntero a dirección. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
	char ipstr[INET6_ADDRSTRLEN]; // string para la dirección en formato texto
	int port; // para almacenar el número de puerto al analizar estructura devuelta

	if (saddr==NULL) { 
		printf("La dirección está vacía\n");
	} else {
		printf("\tFamilia de direcciones: "); fflush(stdout);
		if (saddr->ss_family == AF_INET6) { //IPv6
			printf("IPv6\n");
			// apuntamos a la estructura con saddr_ipv6 (el typecast evita el warning), así podemos acceder al resto de campos a través de este puntero sin más typecasts
			saddr_ipv6=(struct sockaddr_in6 *)saddr;
			// apuntamos a donde está realmente la dirección dentro de la estructura
			addr = &(saddr_ipv6->sin6_addr);
			// obtenemos el puerto, pasando del formato de red al formato local
			port = ntohs(saddr_ipv6->sin6_port);
		} else if (saddr->ss_family == AF_INET) { //IPv4
			printf("IPv4\n");
			saddr_ipv4 = (struct sockaddr_in *)saddr;
			addr = &(saddr_ipv4->sin_addr);
			port = ntohs(saddr_ipv4->sin_port);
		} else {
			fprintf(stderr, "familia desconocida\n");
			exit(1);
		}
		//convierte la dirección ip a string 
		inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
		printf("\tDirección (interpretada según familia): %s\n", ipstr);
		printf("\tPuerto (formato local): %d \n", port);
	}
}
