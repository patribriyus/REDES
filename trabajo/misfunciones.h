/****************************************************************************/
/* Plantilla para cabeceras de funciones del cliente (rcftpclient)          */
/* Plantilla $Revision: 1.7 $ */
/* Autor: Álvarez Peiro, Sergio */
/* Autor: Briones Yus, Patricia */
/****************************************************************************/

/**
 * Devuelve el Mensaje en el formato correcto
 *
 * @param[in/out]	mensaje					
 * @param[in]		version					Version RCFTP_VERSION_1; otro valor es invalido.
 * @param[in]		flags					Mascara de bits
 * @param[in]		sum						Checksum calculado con xsum
 * @param[in]		numseq					Número de secuencia, medido en bytes
 * @param[in]		next					Longitud de datos válidos, no cabeceras
 * @param[in]		len						Longitud de datos válidos, no cabeceras
 */
struct rcftp_msg construirMensajeRCFTP(struct rcftp_msg mensaje, uint8_t version, uint8_t flags, uint16_t sum, uint32_t numseq, uint32_t next, uint16_t len);


/**
 * Determina si un mensaje es válido o no
 *
 * @param[in] recvbuffer Mensaje a comprobar
 * @return 1: es el esperado; 0: no es el esperado
 */
int esMensajeValido(struct rcftp_msg recvbuffer); 


/**
 * Determina si la respuesta recibida es la esperada
 *
 * @param[in] mensaje Mensaje a comprobar
 * @param[in] respuesta Mensaje a comprobar
 */
int esLaRespuestaEsperada(struct rcftp_msg mensaje,struct rcftp_msg respuesta); 


/**
 * Recibe un mensaje (y hace las verificaciones de error oportunas) 
 *
 * @param[in] socket Descriptor de socket
 * @param[out] buffer Espacio donde almacenar lo recibido
 * @param[in] buflen Longitud del buffer
 * @param[out] remote Dirección de la que hemos recibido
 * @param[out] remotelen Longitud de la dirección de la que hemos recibido
 * @return Tamaño del mensaje recibido
 */
ssize_t recibirMensaje(int socket, struct rcftp_msg *buffer, int buflen, struct sockaddr_storage *remote, socklen_t *remotelen, unsigned int flags);


/**
 * Envía un mensaje a la dirección especificada
 *
 * @param[in] s Socket
 * @param[in] sendbuffer Mensaje a enviar
 * @param[in] remote Dirección a la que enviar
 * @param[in] remotelen Longitud de la dirección especificada
 * @param[in] flags Flags del programa
 */
void enviarMensaje(int s, struct rcftp_msg sendbuffer, struct sockaddr_storage remote, socklen_t remotelen, unsigned int flags);


/**
 * @param[in] a primer número para comparar
 * @param[in] b segundo número para comparar
 * @return Número mayor entre a y b
 */
int max(int a, int b);


/**
 * @param[in] a primer número para comparar
 * @param[in] b segundo número para comparar
 * @return Número menor entre a y b
 */
int min(int a, int b);


/**
 * Obtiene la estructura de direcciones del servidor
 *
 * @param[in] dir_servidor String con la dirección de destino
 * @param[in] servicio String con el servicio/número de puerto
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Dirección de estructura con la dirección del servidor
 */
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose);


/**
 * Imprime una estructura sockaddr_in o sockaddr_in6 almacenada en sockaddr_storage
 *
 * @param[in] saddr Estructura de dirección
 */
void printsockaddr(struct sockaddr_storage * saddr);


/**
 * Configura el socket
 *
 * @param[in] servinfo Estructura con la dirección del servidor
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Descriptor del socket creado
 */
int initsocket(struct addrinfo *servinfo, char f_verbose);


// ALGORITMOS


/**
 * Algoritmo 1 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_basico(int socket, struct addrinfo *servinfo);


/**
 * Algoritmo 2 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_stopwait(int socket, struct addrinfo *servinfo);


/**
 * Algoritmo 3 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 * @param[in] window Tamaño deseado de la ventana deslizante
 */
void alg_ventana(int socket, struct addrinfo *servinfo,int window);


