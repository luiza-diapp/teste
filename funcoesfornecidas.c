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
 
int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}


// usando long long pra (tentar) sobreviver ao ano 2038
long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}
 

int protocolo_e_valido(char* buffer, int tamanho_buffer) {
    if (tamanho_buffer < 5) return 0; // tamanho mínimo de um frame

    unsigned char marcador = buffer[0];
    unsigned char tamanho_dados = buffer[1];

    // Verifica se marcador está correto
    if (marcador != 0x7E) return 0;

    // Verifica se tamanho de dados é aceitável (0 a 127)
    if (tamanho_dados > 127) return 0;

    // Verifica se o buffer tem o tamanho mínimo exigido pelo frame
    if (tamanho_buffer < (5 + tamanho_dados)) return 0;

    return 1; // Tudo certo
}
 
// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer) {
    long long comeco = timestamp();
    struct timeval timeout = { .tv_sec = timeoutMillis/1000, .tv_usec = (timeoutMillis%1000) * 1000 };
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    int bytes_lidos;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        if (protocolo_e_valido(buffer, bytes_lidos)) { return bytes_lidos; }
    } while (timestamp() - comeco <= timeoutMillis);
    return -1;
}


int envia(int socket, const char* destino_mac_str, const unsigned char* dados, int tamanho) {
    struct sockaddr_ll sa;
    struct ifreq ifr;
    unsigned char destino_mac[6];

    // converter string MAC para bytes
    sscanf(destino_mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &destino_mac[0], &destino_mac[1], &destino_mac[2],
           &destino_mac[3], &destino_mac[4], &destino_mac[5]);

    // nome da interface fixa (ou passe como argumento depois)
    strncpy(ifr.ifr_name, "enx00e04c2807e3", IFNAMSIZ - 1); // <-- AJUSTE para sua interface

    if (ioctl(socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sll_ifindex = ifr.ifr_ifindex;
    sa.sll_halen = ETH_ALEN;
    memcpy(sa.sll_addr, destino_mac, 6);

    return sendto(socket, dados, tamanho, 0, (struct sockaddr*)&sa, sizeof(sa));
}

int recebe(int socket, unsigned char* dados, char* origem_mac_str) {
    struct sockaddr_ll sa;
    socklen_t sa_len = sizeof(sa);
    unsigned char buffer[2048];

    int lidos = recvfrom(socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&sa, &sa_len);
    if (lidos < 0) {
        perror("recvfrom");
        return -1;
    }

    memcpy(dados, buffer, lidos);

    if (origem_mac_str) {
        sprintf(origem_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                sa.sll_addr[0], sa.sll_addr[1], sa.sll_addr[2],
                sa.sll_addr[3], sa.sll_addr[4], sa.sll_addr[5]);
    }

    return lidos;
}
