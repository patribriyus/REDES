/* $Revision: 2f87fa110238e0fdaa9cd874c0195b05a191e79c $ */

//importaci�n de funciones, constantes, etc.
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>


//definici�n de constantes
#define MAX_BUFF_SIZE 1000 //establecemos este valor m�ximo para el buffer


// cabeceras de funciones
struct addrinfo* obtener_struct_direccion(char *nodo, char *servicio, char f_verbose);
void printsockaddr(struct sockaddr_storage * saddr);
int initsocket(struct addrinfo *servinfo, char f_verbose);


/***************************** MAIN ******************************/
int main(int argc, char * argv[]) {

	//declaraci�n de variables propias del programa principal (locales a main)
	char f_verbose=1; //flag, 1: imprimir por pantalla informaci�n extra
	const char fin = 4; // car�cter ASCII end of transmission (EOT) para indicar fin de transmisi�n
	struct addrinfo * servinfo; // puntero a estructura de direcci�n destino
	int sock; // descriptor del socket
	char msg[MAX_BUFF_SIZE]; // buffer donde almacenar lo le�do y enviarlo
	ssize_t len, // tama�o de lo le�do por la entrada est�ndar (size_t con signo)
			sentbytes; // tama�o de lo enviado (size_t con signo)
	uint32_t num; // variable donde anotar el n�mero de vocales

	//verificaci�n del n�mero de par�metros:
	if (argc != 3) {
		printf("N�mero de par�metros incorrecto \n");
		printf("Uso : %s servidor puerto/servicio\n", argv[0]);
		exit(1); //sale del programa indicando salida incorrecta (1)
	}

	// obtiene estructura de direccion
	servinfo=obtener_struct_direccion(argv[1], argv[2], f_verbose);

	// crea un extremo de la comunicaci�n con la primera de las direcciones de servinfo e inicia la conexi�n con el servidor. Devuelve el descriptor del socket
	sock = initsocket(servinfo, f_verbose);

	// cuando ya no se necesite, hay que liberar la memoria din�mica usada para la direcci�n
	freeaddrinfo(servinfo);
	servinfo=NULL; // como ya no tenemos la memoria, dejamos de apuntarla para evitar acceder a ella por error

	// bucle que lee texto del teclado y lo env�a al servidor
	printf("\nTeclea el texto a enviar y pulsa <Enter>, o termina con <Ctrl+d>\n");
	while ((len = read(0, msg, MAX_BUFF_SIZE)) > 0) { 
		// read lee del teclado hasta que se pulsa INTRO, almacena lo le�do en msg y devuelve la longitud en bytes de los datos le�dos
		if (f_verbose) 
			printf("  Le�dos %zd bytes\n",len);

		if ((sentbytes=sendto(sock,msg,len,0,servinfo->ai_addr,servinfo->ai_addrlen)) < 0) { //env�a datos al socket
			perror("Error de escritura en el socket");
			exit(1);
		} else { if (f_verbose) 
			printf("  Enviados correctamente %zd bytes \n",sentbytes);
		}
		// en caso de que el socket sea cerrado por el servidor, al llamar a send se genera una se�al SIGPIPE, que como en este c�digo no se captura, hace que finalice el programa SIN mensaje de error
		// Las se�ales se estudian en la asignatura Sistemas Operativos

		printf("Teclea el texto a enviar y pulsa <Enter>, o termina con <Ctrl+d>\n");
	}

	//se env�a una marca de finalizaci�n:
	if (sendto(sock,&fin,1,0,servinfo -> ai_addr,servinfo -> ai_addrlen) < 0) {
		perror("Error de escritura en el socket");
		exit(1);
	}
	if (f_verbose) {
	  printf("Enviada correctamente la marca de finalizaci�n.\nEsperando respuesta del servidor...");
	  fflush(stdout);
	}

	//recibe del servidor el n�mero de vocales recibidas:
	if (recvfrom(sock,&num,4,0,servinfo -> ai_addr,&servinfo -> ai_addrlen) < 0) {
		perror("Error de lectura en el socket");
		exit(1);
	}
	printf(" hecho\nEl texto enviado conten�a en total %d vocales\n", num);
	//convierte el entero largo sin signo desde el orden de bytes de la red al del host

	close(sock); //cierra la conexi�n del socket:
	if(f_verbose)
	  printf("Socket cerrado\n");

	exit(0); //sale del programa indicando salida correcta (0)

}


/************************** initsocket ***************************/
//funci�n que crea la conexi�n y se conecta al servidor
int initsocket(struct addrinfo *servinfo, char f_verbose){
	int sock;

	printf("\nSe usar� �NICAMENTE la primera direcci�n de la estructura\n");

	//crea un extremo de la comunicaci�n y devuelve un descriptor
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
 * Funci�n que, en base a ciertos par�metros, devuelve una estructura de direcciones rellenada con al menos una direcci�n que cumpla los par�metros especificados. El �ltimo par�metro sirve para que muestre o no los printfs
 */
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose){
	struct addrinfo hints, //estructura hints para especificar la solicitud
					*servinfo; // puntero al addrinfo devuelto
	int status; // indica la finalizaci�n correcta o no de la llamada getaddrinfo
	int numdir=1; // contador de estructuras de direcciones en la lista de direcciones de servinfo
	struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo

	// genera una estructura de direcci�n con especificaciones de la solicitud
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
   
	if (f_verbose) { printf("\tTipo de comunicaci�n: "); fflush(stdout);}
	hints.ai_socktype= SOCK_STREAM; // especificar tipo de socket 
	if (f_verbose) { 
		switch (hints.ai_socktype) {
			case SOCK_STREAM: printf("flujo (TCP)\n"); break;
			case SOCK_DGRAM: printf("datagrama (UDP)\n"); break;
			default: printf("no convencional (%d)\n",hints.ai_socktype); break;
		}
	}
   
	// pone flags espec�ficos dependiendo de si queremos la direcci�n como cliente o como servidor
	if (dir_servidor!=NULL) {
		// si hemos especificado dir_servidor, es que somos el cliente y vamos a conectarnos con dir_servidor
		if (f_verbose) printf("\tNombre/direcci�n del equipo: %s\n",dir_servidor); 
	} else {
		// si no hemos especificado, es que vamos a ser el servidor
		if (f_verbose) printf("\tNombre/direcci�n del equipo: ninguno (seremos el servidor)\n"); 
		hints.ai_flags= AI_PASSIVE; // poner flag para que la IP se rellene con lo necesario para hacer bind
	}
	if (f_verbose) printf("\tServicio/puerto: %s\n",servicio);
	

	// llamada a getaddrinfo para obtener la estructura de direcciones solicitada
	// getaddrinfo pide memoria din�mica al SO, la rellena con la estructura de direcciones, y escribe en servinfo la direcci�n donde se encuentra dicha estructura
	// la memoria *din�mica* creada dentro de una funci�n NO se destruye al salir de ella. Para liberar esta memoria, usar freeaddrinfo()
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
			printf("    Direcci�n %d:\n",numdir);
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
	struct sockaddr_in  *saddr_ipv4; // puntero a estructura de direcci�n IPv4
	// el compilador interpretar� lo apuntado como estructura de direcci�n IPv4
	struct sockaddr_in6 *saddr_ipv6; // puntero a estructura de direcci�n IPv6
	// el compilador interpretar� lo apuntado como estructura de direcci�n IPv6
	void *addr; // puntero a direcci�n. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
	char ipstr[INET6_ADDRSTRLEN]; // string para la direcci�n en formato texto
	int port; // para almacenar el n�mero de puerto al analizar estructura devuelta

	if (saddr==NULL) { 
		printf("La direcci�n est� vac�a\n");
	} else {
		printf("\tFamilia de direcciones: "); fflush(stdout);
		if (saddr->ss_family == AF_INET6) { //IPv6
			printf("IPv6\n");
			// apuntamos a la estructura con saddr_ipv6 (el typecast evita el warning), as� podemos acceder al resto de campos a trav�s de este puntero sin m�s typecasts
			saddr_ipv6=(struct sockaddr_in6 *)saddr;
			// apuntamos a donde est� realmente la direcci�n dentro de la estructura
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
		//convierte la direcci�n ip a string 
		inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
		printf("\tDirecci�n (interpretada seg�n familia): %s\n", ipstr);
		printf("\tPuerto (formato local): %d \n", port);
	}
}
