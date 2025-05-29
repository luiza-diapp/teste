#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/ether.h>

 
int cria_raw_socket(char* nome_interface_rede);

// usando long long pra (tentar) sobreviver ao ano 2038
long long timestamp() ;

int protocolo_e_valido(char* buffer, int tamanho_buffer) ;

// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer) ;

int envia_servidor(int socket, const char* destino_mac_str, const unsigned char* dados, int tamanho);

int envia_cliente(int socket, const char* destino_mac_str, const unsigned char* dados, int tamanho);

int recebe(int socket, unsigned char* dados, char* origem_mac_str) ;
