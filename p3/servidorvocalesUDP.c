/* $Revision: 2f87fa110238e0fdaa9cd874c0195b05a191e79c $ */

//importaci�n de funciones, constantes, variables, etc.
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

//definici�n de constantes
#define EOT 4               //car�cter ASCII end of transmision
#define BUFF_SIZE 1000       //establecemos el tama�o del buffer


//definici�n de funciones
struct addrinfo* obtener_struct_direccion(char *nodo, char *servicio, char f_verbose);
void printsockaddr(struct sockaddr_storage * saddr);
int establecer_servicio(struct addrinfo *servinfo, char f_verbose);
uint32_t countVowels(char msg[], size_t s);


/***************************** MAIN ******************************/
int main(int argc, char * argv[]) {
  
	//declaraci�n de variables propias del programa principal (locales a main)
	char f_verbose=1; //flag, 1: imprimir por pantalla informaci�n extra
	struct addrinfo* servinfo; // direcci�n propia (servidor)
	int sock; // descriptores de socket 
	char msg[BUFF_SIZE]; // espacio para almacenar lo recibido
	ssize_t readbytes; // numero de bytes recibidos
	uint32_t num, netNum; // contador de vocales en formato local y de red
	struct sockaddr_storage caddr; // direcci�n del cliente
	socklen_t clen; // longitud de la direcci�n 

	//verificaci�n del n�mero de par�metros:
	if (argc != 2) {
		printf("N�mero de par�metros incorrecto \n");
		printf("Uso : %s puerto\n", argv[0]);
		exit(1);
	}

	// obtiene estructura de direccion
	servinfo=obtener_struct_direccion(NULL, argv[1], f_verbose);

	// crea un extremo de la comunicaci�n. Devuelve el descriptor del socket
	sock = establecer_servicio(servinfo,f_verbose);

	// cuando ya no se necesite, hay que liberar la memoria din�mica usada para la direcci�n
	freeaddrinfo(servinfo);
	servinfo=NULL; // como ya no tenemos la memoria, dejamos de apuntarla para evitar acceder a ella por error

	// bucle infinito para atender conexiones una tras otra
	while (1) {
		// bucle de contar vocales hasta recibir marca de fin
		num = 0;
		do {
		  if ((readbytes = recvfrom(sock,msg,BUFF_SIZE,0,(struct sockaddr *)&caddr,&clen)) < 0) {
		    perror("Error de lectura en el socket");
		    exit(1);
		    
		  }
		  printf("Mensaje recibido: ");fflush(stdout);
		  write(1, msg, readbytes); //muestra en pantalla (salida est�ndar 1) el mensaje recibido
		  // evitamos usar printf por si lo recibido no es texto o no acaba con \0
		  num += countVowels(msg, readbytes);
		  printf("Vocales contadas hasta el momento: %d\n",num);
		  
		// condici�n de final: haber recibido al menos un byte y que lo pen�ltimo sea la marca de fin
		// se comprueba lo pen�ltimo porque lo �ltimo siempre es un <intro>
		} while ((readbytes>0) && (msg[readbytes - 1] != EOT)); 

		printf("\nMarca de fin recibida\n");
		printf("Contadas %d vocales\n", num); //muestra las vocales recibidas
		netNum = htonl(num); //convierte el entero largo sin signo hostlong desde el orden de bytes del host al de la red
		//envia al cliente el n�mero de vocales recibidas:
		if (sendto(sock, &netNum,sizeof netNum,0,(struct sockaddr*)&caddr,clen) < 0) {
		  perror("Error de escritura en el socket");
		  exit(1);
		  
		}
		if(f_verbose)
		  printf("Enviado el n�mero de vocales contadas\n");

		//cierra la conexi�n con el cliente
		close(sock);
		if(f_verbose)
			printf("Cerrada la conexi�n con el cliente\n");

	}
	exit(0);
  
}

/***************************** establecer_servicio ******************************/
//funci�n que crea la conexi�n y espera conexiones entrantes 
int establecer_servicio(struct addrinfo *servinfo, char f_verbose){
	int sock;
	
	printf("\nSe usar� �NICAMENTE la primera direcci�n de la estructura\n");

	//crea un extremo de la comunicaci�n y devuelve un descriptor
	if (f_verbose) { printf("Creando el socket (socket)... "); fflush(stdout); }
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if (sock < 0) {
		perror("Error en la llamada socket: No se pudo crear el socket");
		/*muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
		exit(1);
	}
	if (f_verbose) { printf("hecho\n"); }

	
	//asocia el socket con un puerto
	if (f_verbose) { printf("Asociando socket a puerto (bind)... "); fflush(stdout); }
	if (bind(sock,servinfo -> ai_addr,servinfo -> ai_addrlen) < 0) {
	  perror("Error asociando el socket");
	  exit(1);
	}
	if (f_verbose) { printf("hecho\n"); }
	
	return sock;
}

//funci�n que cuenta vocales
uint32_t countVowels(char msg[], size_t s) {
	uint32_t result = 0;
	size_t i;
	for (i = 0; i < s; i++)
	  if (msg[i] == 'a' || msg[i] == 'A' ||
	    msg[i] == 'e' || msg[i] == 'E' ||
	    msg[i] == 'i' || msg[i] == 'I' ||
	    msg[i] == 'o' || msg[i] == 'O' ||
	    msg[i] == 'u' || msg[i] == 'U') result++;
	return result;
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
