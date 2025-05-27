#include <stdio.h>
#include <string.h>
#include "protocolo.h"
#include "tabuleiroservidor.h"

extern int cria_raw_socket(const char* interface);
extern int envia(int socket, const char* destino, const unsigned char* dados, int tamanho);
extern int recebe(int socket, unsigned char* dados, char* origem);
extern int protocolo_e_valido(char* buffer, int tamanho_buffer);


int lida_tabuleiro (char* buffer, Frame f){

    printf("\nTABULEIRO INICIAL (sem tesouros):\n\n");
    for (int i = TAM - 1; i >= 0; i--) {
        for (int j = 0; j < TAM; j++) {
            uint8_t valor = f.dados[i * TAM + j];
            switch (valor) {
                case VAZIO:    printf(" . "); break;
                case VISITADO: printf(" X "); break;
                case JOGADOR:  printf(" P "); break;
                default:       printf(" ? "); break;
            }
        }
        printf("\n");
    }
}

int confirma_que_recebeu(int sock, char mac_origem[18], Frame f){
    uint8_t vazio[] = {0};
    Frame ack = empacotar(TIPO_ACK, f.sequencia, vazio, 0);

    // Enviar o ACK de volta para o MAC de origem
    envia(sock, mac_origem, (unsigned char*)&ack, sizeof(Frame));
    printf("ACK enviado para %s\n", mac_origem);
}



int main() {
    int sock = cria_raw_socket("enx00e04c2807e3");  // ajuste para sua interface
    if (sock < 0) {
        perror("Erro ao criar raw socket");
        return 1;
    }

    unsigned char buffer[sizeof(Frame)];
    char mac_origem[18];
    while (1) {
        int lidos = recebe(sock, buffer, mac_origem);
        if ((lidos > 0)  && (protocolo_e_valido((char*)buffer, lidos))) {
            Frame f;
            if (desempacotar(&f, buffer, lidos) == 0) {
                printf("Recebido tipo: %d de %s\n", f.tipo, mac_origem);
                if (f.tipo != 0) confirma_que_recebeu(sock, mac_origem, f);
               	if (f.tipo == 16)
                    lida_tabuleiro(buffer, f);
            }
        else
            printf("Erro ao desempacotar frame.\n");
        }
    }

    /*
    uint8_t dados_dummy[] = {0}; // sem dados
    Frame f = empacotar(TIPO_CIMA, 0, dados_dummy, 0);

    char mac_destino[] = "88:6f:d4:fc:af:14";// MAC do servidor (ajuste para o real)

    if (envia(sock, mac_destino, (unsigned char*)&f, sizeof(f)) < 0) {
        perror("Erro ao enviar frame");
    } else {
        printf("Movimento enviado (cima).\n");
    }
    
    */

    return 0;
}



