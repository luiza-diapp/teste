#include <stdio.h>
#include <string.h>
#include "protocolo.h"
#include "tabuleiroservidor.h"

extern int cria_raw_socket(const char* interface);
extern int recebe(int socket, unsigned char* dados, char* origem);
extern int envia(int socket, const char* destino, const unsigned char* dados, int tamanho);
extern int protocolo_e_valido(char* buffer, int tamanho_buffer);

// Retorna vetor de 64 bytes representando o tabuleiro
void serializar_tabuleiro(const Jogo* jogo, uint8_t* buffer) {
    for (int i = 0; i < TAM; i++) {
        for (int j = 0; j < TAM; j++) {
            Estado celula = jogo->tabuleiro[i][j];
            if (celula == TESOURO) celula = VAZIO; // Esconde os tesouros!
            buffer[i * TAM + j] = (uint8_t)celula;
        }
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
    int sock = cria_raw_socket("enp1s0");  // ajuste para sua interface
    if (sock < 0) {
        perror("Erro ao criar raw socket");
        return 1;
    }

    unsigned char buffer[sizeof(Frame)];
    char mac_origem[18];

    Jogo* jogo = criar_jogo();
    inicializar_tabuleiro(jogo);

    uint8_t mapa_serializado[TAM * TAM];
    serializar_tabuleiro(jogo, mapa_serializado);

    // Empacotar e enviar o frame com o mapa inicial
    Frame f = empacotar(TIPO_TABULEIRO, 0, mapa_serializado, TAM * TAM);
    envia(sock, "MAC_DO_CLIENTE", (unsigned char*)&f, sizeof(Frame));

    while (1) {
        int lidos = recebe(sock, buffer, mac_origem);
        if ((lidos > 0) && (protocolo_e_valido((char*)buffer, lidos))) {
            Frame f;
            if (desempacotar(&f, buffer, lidos) == 0) {
                printf("Recebido tipo: %d de %s\n", f.tipo, mac_origem);
                if (f.tipo != 0) confirma_que_recebeu(sock, mac_origem, f);
            } else {
                printf("Erro ao desempacotar frame.\n");
            }
        }
    }

    return 0;
}
