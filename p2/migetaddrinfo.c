/* $Revision: f75f0d1830c299032471d912a1a204d9499f6dce $ */
// Compilar en hendrix mediante: gcc -Wall -o migetaddrinfo -lsocket -lnsl migetaddrinfo.c
// Compilar en lab mediante: gcc -Wall -o migetaddrinfo migetaddrinfo.c

// importaci�n de funciones, constantes, etc.
// el preprocesador sustituye cada include por contenido del fichero referenciado
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

// cabeceras de funciones (importante que aparezcan antes de ser usadas)
struct addrinfo* obtener_struct_direccion(char *nodo, char *servicio, char f_verbose);
void printsockaddr(struct sockaddr_storage * saddr);


/***************************** MAIN ******************************/
/* argc indica el n�mero de argumentos que se han usado en el la l�nea de comandos. argv es un vector de cadenas de caracteres.  El elemento argv[0] contiene el nombre del programa y as�, sucesivamente.*/
int main(int argc, char * argv[]){ 	
	char f_verbose=1; //flag, 1: imprimir informaci�n por pantalla
	struct addrinfo* direccion; // puntero (no inicializado!) a estructura de direcci�n

	//verificaci�n del n�mero de par�metros:
	if ((argc!=2) && (argc!=3)) {
		printf("N�mero de par�metros incorrecto \n");
		printf("Uso: %s [servidor] <puerto/servicio>\n", argv[0]);
		exit(1); //sale del programa indicando salida incorrecta (1)
	} else if (argc==3) {
		// devuelve la estructura de direcci�n al equipo y servicio solicitado
		direccion=obtener_struct_direccion(argv[1], argv[2], f_verbose);
	} else if (argc==2) {
		// devuelve la estructura de direcci�n del servicio solicitado asumiendo que vamos a actuar como servidor
		direccion=obtener_struct_direccion(NULL,argv[1], f_verbose);
	}
	
	// cuando ya no se necesite hay que liberar la memoria din�mica obtenida en getaddrinfo() mediante freeaddrinfo()
	if (f_verbose) { printf("Devolviendo al sistema la memoria usada por servinfo (ya no se va a usar)... "); fflush(stdout);}
	freeaddrinfo(direccion);
	if (f_verbose) printf("hecho\n");
	direccion=NULL; // como ya no tenemos la memoria, dejamos de apuntarla para evitar acceder a ella por error

	// sale del programa indicando salida correcta (0)
	exit(0);
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

