/****************************************************************************/
/* Plantilla para implementación de funciones del cliente (rcftpclient)     */
/* $Revision: 1.6 $ */
/* Aunque se permite la modificación de cualquier parte del código, se */
/* recomienda modificar solamente este fichero y su fichero de cabeceras asociado. */
/****************************************************************************/

/**************************************************************************/
/* INCLUDES                                                               */
/**************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "rcftp.h" // Protocolo RCFTP
#include "rcftpclient.h" // Funciones ya implementadas
#include "multialarm.h" // Gestión de timeouts
#include "vemision.h" // Gestión de ventana de emisión
#include "misfunciones.h"

/**************************************************************************/
/* VARIABLES GLOBALES                                                     */
/**************************************************************************/
#define S_SYSERROR 2 /**< Flag de salida por error de sistema */
#define F_VERBOSE	0x1	/**< Flag para mostrar detalles en la salida estándar */
#define ANSI_COLOR_RED     "\x1b[31m" /**< Pone terminal a rojo */
#define ANSI_COLOR_GREEN   "\x1b[32m" /**< Pone terminal a verde */
#define ANSI_COLOR_YELLOW  "\x1b[33m" /**< Pone terminal a amarillo */
#define ANSI_COLOR_BLUE    "\x1b[34m" /**< Pone terminal a azul */
#define ANSI_COLOR_MAGENTA "\x1b[35m" /**< Pone terminal a magenta */
#define ANSI_COLOR_CYAN    "\x1b[36m" /**< Pone terminal a turquesa */
#define ANSI_COLOR_RESET   "\x1b[0m" /**< Desactiva color del terminal */

char* autores="Autor: Alvarez Peiro, Sergio\nAutor: Briones Yus, Patricia"; // dos autores

// variable para indicar si mostrar información extra durante la ejecución
// como la mayoría de las funciones necesitaran consultarla, la definimos global
extern char verb;


// variable externa que muestra el número de timeouts vencidos
// Uso: Comparar con otra variable inicializada a 0; si son distintas, tratar un timeout e incrementar en uno la otra variable
extern volatile const int timeouts_vencidos;


/**************************************************************************/
/* Construye el mensaje RCFTP */
/**************************************************************************/
struct rcftp_msg construirMensajeRCFTP(struct rcftp_msg mensaje, uint8_t version, uint8_t flags, uint16_t sum, uint32_t numseq, uint32_t next, uint16_t len) {
	mensaje.version = version;
	mensaje.numseq = htonl(numseq);
	mensaje.flags = flags;
	mensaje.len = htons(len);
	mensaje.next = htonl(next);
	mensaje.sum = sum;
	mensaje.sum = xsum((char*)&mensaje,sizeof(mensaje));
	return mensaje;
}


/**************************************************************************/
/* Verifica version, next y checksum del mensaje */
/**************************************************************************/
int esMensajeValido(struct rcftp_msg recvbuffer) {
    int esperado = 1;
    if (recvbuffer.version!=RCFTP_VERSION_1) { // versión incorrecta
        esperado = 0;
        fprintf(stderr,"Error: recibido un mensaje con versión incorrecta\n");
    }
    if (recvbuffer.next == 0) { // next incorrecto
        esperado = 0;
        fprintf(stderr,"Error: recibido un mensaje con NEXT incorrecto\n");
    }
    if (issumvalid(&recvbuffer,sizeof(recvbuffer))==0) { // checksum incorrecto
        esperado = 0;
        fprintf(stderr,"Error: recibido un mensaje con checksum incorrecto\n");
    }
    return esperado;	
}


/**************************************************************************/
/* Verifica si es la respuesta esperada */
/**************************************************************************/
int esLaRespuestaEsperada(struct rcftp_msg mensaje,struct rcftp_msg respuesta) {
    int esperado = 1;
    if ((ntohl(mensaje.numseq) + ntohs(mensaje.len)) > ntohl(respuesta.next)) {
        esperado = 0;
        fprintf(stderr,"Error: Next recibido incorrecto\n");
    }
    if (respuesta.flags == F_BUSY ||respuesta.flags == F_ABORT) {
        esperado = 0;
        fprintf(stderr,"Error: ocupado o abortado\n");
    }
    if (mensaje.flags == F_FIN &&respuesta.flags != F_FIN) {
        esperado = 0;
        fprintf(stderr,"Error: fines distintos\n");
    }
    return esperado;
} 


/**************************************************************************/
/* Recibe mensaje (y hace las verificaciones oportunas) */
/**************************************************************************/
ssize_t recibirMensaje(int socket, struct rcftp_msg *buffer, int buflen, struct sockaddr_storage *remote, socklen_t *remotelen, unsigned int flags) {
	struct sockaddr* address = (struct sockaddr *)remote;
	ssize_t recvsize;
	*remotelen = sizeof(*remote);
	char* cadena = (char *)buffer;
	recvsize = recvfrom(socket,cadena,buflen,0,address,remotelen);
	if (recvsize == -1) {
		return recvsize;
	}
	if (recvsize < 0 && errno!=EAGAIN) {
		perror("Error en recvfrom: ");
		exit(S_SYSERROR);
	} 
	else if (*remotelen>sizeof(*remote)) {
		fprintf(stderr,"Error: la dirección del cliente ha sido truncada\n");
		exit(S_SYSERROR);
	}
	if (flags & F_VERBOSE) {
		printf("Mensaje RCFTP " ANSI_COLOR_GREEN "recibido" ANSI_COLOR_RESET ":\n");
		print_rcftp_msg(buffer,sizeof(*buffer));
	} 
	return recvsize;
}


/**************************************************************************/
/* Envía un mensaje a la dirección especificada */
/**************************************************************************/
void enviarMensaje(int soc, struct rcftp_msg sendbuffer, struct sockaddr_storage remote, socklen_t remotelen, unsigned int flags) {
	struct sockaddr* addr = (struct sockaddr *)&remote;
	ssize_t sentsize;
	const char* cadena = (char *)&sendbuffer;
	int msgLen = sizeof(sendbuffer);	
	sentsize=sendto(soc,cadena,msgLen,0,addr,(int)remotelen);
	if ((sentsize != msgLen)) {
		if (sentsize != -1) {
			fprintf(stderr,"Error: enviados %d bytes de un mensaje de %d bytes\n",(int)sentsize,(int)msgLen);
		}
		else {
			perror("Error en sendto\n");
			exit(S_SYSERROR);
		}
	} 
	// print response if in verbose mode
	if (flags & F_VERBOSE) {
		printf("Mensaje RCFTP " ANSI_COLOR_MAGENTA "enviado" ANSI_COLOR_RESET ":\n");
		print_rcftp_msg(&sendbuffer,sizeof(sendbuffer));
	} 
}


/**************************************************************************/
/* Devuelve el número mayor */
/**************************************************************************/
int max(int a, int b) {
	if (a > b) { return a; }
	else { return b; }
}


/**************************************************************************/
/* Devuelve el número menor */
/**************************************************************************/
int min(int a, int b) {
	if (a < b) { return a; }
	else { return b; }
}


/**************************************************************************/
/*  Devuelve 1 si hay espacio en la ventana de emisión  */
/**************************************************************************/
int espacioLibreEnVentanaEmision() {
	if (getfreespace() > 0) {
		return 1;
	}
	else {
		return 0;
	}
}


/**************************************************************************/
/* Obtiene la estructura de direcciones del servidor */
/**************************************************************************/
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose) {
	struct addrinfo hints,		// estructura hints para especificar la solicitud
					*servinfo;	// puntero al addrinfo devuelto
	int status; 				// indica la finalización correcta o no de la llamada getaddrinfo
	int numdir=1; 				// contador de estructuras de direcciones en la lista de direcciones de servinfo
	struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo
	// genera una estructura de dirección con especificaciones de la solicitud
	if (f_verbose) {
		printf("1 - Especificando detalles de la estructura de direcciones a solicitar... \n");
	}
	// sobreescribimos con ceros la estructura para borrar cualquier dato que pueda malinterpretarse
	memset(&hints, 0, sizeof hints); 
	if (f_verbose) {
		printf("\tFamilia de direcciones/protocolos: ");
		fflush(stdout);
	}
	hints.ai_family= AF_UNSPEC; //CON ESTO SE REFIERE A QUE SE DESCONOCE EL PROTOCOLO A USAR // sin especificar: AF_UNSPEC; IPv4: AF_INET; IPv6: AF_INET6; etc.
	if (f_verbose) { 
		switch (hints.ai_family) {
			case AF_UNSPEC: printf("IPv4 e IPv6\n"); break;
			case AF_INET: printf("IPv4)\n"); break;
			case AF_INET6: printf("IPv6)\n"); break;
			default: printf("No IP (%d)\n",hints.ai_family); break;
		}
	}
	if (f_verbose) {
		printf("\tTipo de comunicacion: ");
		fflush(stdout);
	}
	hints.ai_socktype= SOCK_DGRAM; //SIEMPRE SE USARA STREAM // especificar tipo de socket 
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
		if (f_verbose) {
			printf("\tNombre/direccion del equipo: %s\n",dir_servidor);
		}
	}
	else {
		// si no hemos especificado, es que vamos a ser el servidor
		if (f_verbose) printf("\tNombre/direccion del equipo: ninguno (seremos el servidor)\n"); 
		hints.ai_flags= AI_PASSIVE; //WILDCARD UIP ADDRESS // poner flag para que la IP se rellene con lo necesario para hacer bind
	}
	if (f_verbose) {
		printf("\tServicio/puerto: %s\n",servicio);
	}
	// llamada a getaddrinfo para obtener la estructura de direcciones solicitada
	// getaddrinfo pide memoria dinámica al SO, la rellena con la estructura de direcciones, y escribe en servinfo la dirección donde se encuentra dicha estructura
	// la memoria *dinámica* creada dentro de una función NO se destruye al salir de ella. Para liberar esta memoria, usar freeaddrinfo()
	if (f_verbose) {
		printf("2 - Solicitando la estructura de direcciones con getaddrinfo()... ");
		fflush(stdout);
	}
	status = getaddrinfo(dir_servidor,servicio,&hints,&servinfo);
	if (status!=0) {
		fprintf(stderr,"Error en la llamada getaddrinfo: %s\n",gai_strerror(status));
		exit(1);
	} 
	if (f_verbose) {
		printf("hecho\n");
	}
	// imprime la estructura de direcciones devuelta por getaddrinfo()
	if (f_verbose) {
		printf("3 - Analizando estructura de direcciones devuelta... \n");
		direccion=servinfo;
		while (direccion!=NULL) { // bucle que recorre la lista de direcciones
			printf("    Direccion %d:\n",numdir);
			printsockaddr((struct sockaddr_storage*)direccion->ai_addr);
			// "avanzamos" direccion a la siguiente estructura de direccion
			direccion=direccion->ai_next;
			numdir++;
		}
	}
	// devuelve la estructura de direcciones devuelta por getaddrinfo()
	return servinfo;
}
/**************************************************************************/
/* Imprime una direccion */
/**************************************************************************/
void printsockaddr(struct sockaddr_storage * saddr) {
	struct sockaddr_in  *saddr_ipv4; 	// puntero a estructura de dirección IPv4
	// el compilador interpretará lo apuntado como estructura de dirección IPv4
	struct sockaddr_in6 *saddr_ipv6; 	// puntero a estructura de dirección IPv6
	// el compilador interpretará lo apuntado como estructura de dirección IPv6
	void *addr; // puntero a dirección. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
	char ipstr[INET6_ADDRSTRLEN]; 		// string para la dirección en formato texto
	int port; // para almacenar el número de puerto al analizar estructura devuelta
	if (saddr==NULL) { 
		printf("La direccion está vacía\n");
	} 
	else {
		printf("\tFamilia de direcciones: "); fflush(stdout);
		if (saddr->ss_family == AF_INET6) { //IPv6
			printf("IPv6\n");
			// apuntamos a la estructura con saddr_ipv6 (el typecast evita el warning), así podemos acceder al resto de campos a través de este puntero sin más typecasts
			saddr_ipv6=(struct sockaddr_in6 *)saddr;
			// apuntamos a donde está realmente la dirección dentro de la estructura
			addr = &(saddr_ipv6->sin6_addr);
			// obtenemos el puerto, pasando del formato de red al formato local
			port = ntohs(saddr_ipv6->sin6_port);
		}
		else if (saddr->ss_family == AF_INET) { //IPv4
			printf("IPv4\n");
			saddr_ipv4 = (struct sockaddr_in *)saddr;
			addr = &(saddr_ipv4->sin_addr) ;
			port = ntohs(saddr_ipv4->sin_port);
		}
		else {
			fprintf(stderr, "familia desconocida\n");
			exit(1);
		}
		//convierte la dirección ip a string 
		inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
		printf("\tDireccion (interpretada segun familia): %s\n", ipstr);
		printf("\tPuerto (formato local): %d \n", port);
	}
}
/**************************************************************************/
/* Configura el socket, devuelve el socket y servinfo */
/**************************************************************************/
int initsocket(struct addrinfo *servinfo, char f_verbose){
	int sock;
	printf("\nSe usará UNICAMENTE la primera direccion de la estructura\n");
	//crea un extremo de la comunicación y devuelve un descriptor
	if (f_verbose) { printf("Creando el socket (socket)... "); fflush(stdout); }
	sock = socket((servinfo->ai_family), (servinfo->ai_socktype), (servinfo->ai_protocol));
	if (sock < 0) {
		perror("Error en la llamada socket: No se pudo crear el socket");
		/*muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
		exit(1);
	}
	if (f_verbose) { printf("hecho\n"); }
	return sock;
}


/**************************************************************************/
/*  algoritmo 1 (basico)  */
/**************************************************************************/
void alg_basico(int socket, struct addrinfo *servinfo) {
	printf("Comunicación con algoritmo básico\n");
	struct rcftp_msg mensaje;
	struct rcftp_msg respuesta;
	struct sockaddr_storage *remote = (struct sockaddr_storage*)(servinfo->ai_addr);
	int ultimoMensaje = 0
	int ultimoMensajeConfirmado = 0; 
	int nSeq = 0;
	int datosLen = readtobuffer((char*)mensaje.buffer,RCFTP_BUFLEN);
	uint8_t flag = F_NOFLAGS;
	socklen_t remoteLen = servinfo->ai_addrlen;
	if(datosLen < RCFTP_BUFLEN) {
		// Fin de fichero alcanzado
		ultimoMensaje = 1;
		flag = F_FIN;
		printf("alg_basico: Recibido último mensaje\n");
	}
	mensaje = construirMensajeRCFTP(mensaje,RCFTP_VERSION_1,flag,0,nSeq,0,datosLen);
	while (ultimoMensajeConfirmado == 0) {
		// Envía mensaje y espera la respuesta
		enviarMensaje(socket,mensaje,*remote,remoteLen,F_VERBOSE);
		recibirMensaje(socket,&respuesta,sizeof(respuesta),remote,&remoteLen,F_VERBOSE);
		if (esMensajeValido(respuesta) == 1 && esLaRespuestaEsperada(mensaje,respuesta) == 1) {
			// Respuesta válida
			if (ultimoMensaje == 1) {
				// Última respuesta válida
				ultimoMensajeConfirmado = 1;
			}
			else {
				nSeq = nSeq + datosLen;
				datosLen = readtobuffer((char*)mensaje.buffer,RCFTP_BUFLEN);
				if (datosLen < RCFTP_BUFLEN) {
					// Fin de fichero alcanzado
					ultimoMensaje = 1;
					flag = F_FIN;
					printf("alg_basico: Recibido último mensaje\n");
				}
				// Construir siguiente mensaje
				mensaje = construirMensajeRCFTP(mensaje,RCFTP_VERSION_1,flag,0,nSeq,0,datosLen);
			}
		}
	}
}

/**************************************************************************/
/*  algoritmo 2 (stop & wait)  */
/**************************************************************************/
void alg_stopwait(int socket, struct addrinfo *servinfo) {
	printf("Comunicación con algoritmo stop&wait\n");
	signal(SIGALRM,handle_sigalrm);
	struct rcftp_msg mensaje;
    struct rcftp_msg respuesta;
	struct sockaddr_storage *remote = (struct sockaddr_storage*)(servinfo -> ai_addr);
    int ultimoMensaje = 0;
	int ultimoMensajeConfirmado = 0;
	int nSeq = 0;
    int esperar = 0;
    int timeouts_procesados = 0;
    int numDatosRecibidos = 0;
    int datosLen = readtobuffer((char*)mensaje.buffer,RCFTP_BUFLEN);
	int sockflags = fcntl(socket,F_GETFL,0);
    fcntl(socket,F_SETFL,sockflags | O_NONBLOCK);
    socklen_t remotelen = servinfo->ai_addrlen;
    uint8_t flag = F_NOFLAGS;
    if (datosLen < RCFTP_BUFLEN) {
		// Fin de fichero alcanzado
        ultimoMensaje = 1;
        flag = F_FIN;
        printf("alg_stopwait: Recibido último mensaje\n");
    }
    mensaje = construirMensajeRCFTP(mensaje,RCFTP_VERSION_1,flag,0,nSeq,0,datosLen);
    while (ultimoMensajeConfirmado == 0) {
		// Envía mensaje y añade un timeout esperando a recibir la respuesta
        enviarMensaje(socket, mensaje, *remote, remotelen, F_VERBOSE);
        addtimeout();
        esperar = 1;
        while (esperar == 1) {
            numDatosRecibidos = recibirMensaje(socket,&respuesta,sizeof(respuesta),remote,&remotelen,F_VERBOSE);
            if (numDatosRecibidos > 0) {
				// Cancelación del timeout: se ha recibido el mensaje
                canceltimeout();
                esperar = 0;
            }
            if (timeouts_procesados != timeouts_vencidos) {
                esperar = 0;
                ++timeouts_procesados;
            }
        }
        if (esMensajeValido(respuesta) == 1 && esLaRespuestaEsperada(mensaje,respuesta) == 1) {
            if (ultimoMensaje == 1) {
				// Última respuesta válida
                ultimoMensajeConfirmado = 1;
            }
            else {
                nSeq = nSeq + datosLen;
                datosLen = readtobuffer((char*)mensaje.buffer,RCFTP_BUFLEN);
                if (datosLen < RCFTP_BUFLEN) {
					// Fin de fichero alcanzado
                    ultimoMensaje = 1;
                    flag = F_FIN;
                    printf("alg_stopwait: Recibido último mensaje\n");
                }
				// Construir siguiente mensaje
                mensaje = construirMensajeRCFTP(mensaje,RCFTP_VERSION_1,flag,0,nSeq,0,datosLen);
            }
        }
    }
}


/**************************************************************************/
/*  algoritmo 3 (ventana deslizante)  */
/**************************************************************************/
void alg_ventana(int socket, struct addrinfo *servinfo,int window) {
	printf("Comunicación con algoritmo go-back-n\n");
	struct rcftp_msg mensaje;
	struct rcftp_msg respuesta;
	struct sockaddr_storage *remote = (struct sockaddr_storage*)(servinfo->ai_addr);
	int nSeq = 0;
	int timeouts_procesados = 0;
	int ultimoMensajeConfirmado = 0;
	int	numDatosRecibidos = 0;
	int datosLen = -1;
	int minimo = 0;
	int *len;
	int sockflags = fcntl(socket,F_GETFL,0);
	fcntl(socket,F_SETFL,sockflags | O_NONBLOCK);
	handle_sigalrm(SIGALRM);
	socklen_t  remotelen = servinfo->ai_addrlen;
	uint8_t flag = F_NOFLAGS;
	setwindowsize(window);
	while(ultimoMensajeConfirmado == 0) {
		// BLOQUE DE ENVIO: Enviar datos si hay espacio en ventana
		if(espacioLibreEnVentanaEmision()==1 && datosLen != 0 && getfreespace()>=datosLen) {
			nSeq = nSeq + max(datosLen,0);
			datosLen = readtobuffer((char*)mensaje.buffer,min(RCFTP_BUFLEN,window));
			if (datosLen < min(RCFTP_BUFLEN, window)) {
				// Fin de fichero alcanzado
				flag = F_FIN;
				printf("alg_ventana: Recibido último mensaje\n");
			}
			mensaje = construirMensajeRCFTP(mensaje,RCFTP_VERSION_1,flag,0,nSeq,0,datosLen);
			enviarMensaje(socket,mensaje,*remote,remotelen,F_VERBOSE);
			addtimeout();
			addsentdatatowindow((char*)mensaje.buffer,datosLen);
		}
		// BLOQUE DE RECEPCION: Recibir respuesta y procesarla (si existe)
		numDatosRecibidos = recibirMensaje(socket,&respuesta,sizeof(respuesta),remote,&remotelen,F_VERBOSE); 
		if(numDatosRecibidos > 0) {
			if (esMensajeValido(respuesta) == 1 && esLaRespuestaEsperada(mensaje,respuesta) == 1) {
				canceltimeout();	
				freewindow(ntohl(respuesta.next));
				if (respuesta.flags == F_FIN) {
					ultimoMensajeConfirmado = 1;
				}
			}
		}
		// BLOQUE DE PROCESADO DE TIMEOUT
		if (timeouts_procesados != timeouts_vencidos) {
			minimo = min(RCFTP_BUFLEN,window);
			len = &minimo;
			nSeq = getdatatoresend((char*)mensaje.buffer,len);
			mensaje = construirMensajeRCFTP(mensaje,RCFTP_VERSION_1,flag,0,nSeq,0,min(RCFTP_BUFLEN,window));
			print_rcftp_msg(&mensaje, sizeof(mensaje));
			enviarMensaje(socket,mensaje,*remote,remotelen,F_VERBOSE);
			addtimeout();
			timeouts_procesados++;	
		}
	}
}


