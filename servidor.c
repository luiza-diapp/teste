#include <stdio.h>
#include <string.h>
#include "protocolo.h"
#include "tabuleiroservidor.h"
#include "funcoesfornecidass.h"

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
    envia_servidor(sock, mac_origem, (unsigned char*)&ack, sizeof(Frame));
    printf("ACK enviado para %s\n", mac_origem);
}

int envia_nack(int sock, char mac_origem[18], Frame f){
    uint8_t vazio[] = {0};
    Frame ack = empacotar(TIPO_NACK, f.sequencia, vazio, 0);

    // Enviar o ACK de volta para o MAC de origem
    envia_servidor(sock, mac_origem, (unsigned char*)&ack, sizeof(Frame));
    printf("ACK enviado para %s\n", mac_origem);
}

void receber_mensagem_texto(int socket) {
    unsigned char buffer[2048];
    char origem_mac[18];
    Frame f;

    char mensagem[1024];  // Suporta até 1024 caracteres de texto
    int pos = 0;

    while (1) {
        int lidos = recebe(socket, buffer, origem_mac);
        if (lidos <= 0) continue;
        if (!protocolo_e_valido((char*)buffer, lidos)) continue;
        if (desempacotar(&f, buffer, lidos) != 0) continue;

        if (f.tipo == TIPO_TEXTO) {
            if (pos + f.tamanho < sizeof(mensagem)) {
                memcpy(&mensagem[pos], f.dados, f.tamanho);
                pos += f.tamanho;
            } else {
                printf("Erro: mensagem muito grande\n");
                break;
            }
        } else if (f.tipo == TIPO_FIM_ARQUIVO) {
            mensagem[pos] = '\0'; // Finaliza a string
            printf("\n📨 Mensagem recebida:\n%s\n", mensagem);
            break;
        }
    }
}

void enviar_mensagem_texto(int socket, const char* mac_destino, const char* texto) {
    uint8_t sequencia = 0;
    size_t total = strlen(texto);
    size_t enviados = 0;

    while (enviados < total) {
        uint8_t tamanho = (total - enviados > TAM_MAX_DADOS) ? TAM_MAX_DADOS : (total - enviados);
        Frame f = empacotar(TIPO_TEXTO, sequencia, (uint8_t*)&texto[enviados], tamanho);
        envia_servidor(socket, mac_destino, (unsigned char*)&f, sizeof(Frame));
        printf("Enviado: \"%.*s\" (seq %d)\n", tamanho, &texto[enviados], sequencia);
        enviados += tamanho;
        sequencia++;
    }

    // Envia um frame especial indicando fim da mensagem
    Frame fim = empacotar(TIPO_FIM_ARQUIVO, sequencia, NULL, 0);
    envia_servidor(socket, mac_destino, (unsigned char*)&fim, sizeof(Frame));
    printf("Enviado TIPO_FIM_ARQUIVO (seq %d)\n", sequencia);
}

void envia_tabuleiro(int sock, Jogo* jogo){
    uint8_t mapa_serializado[TAM * TAM];
    serializar_tabuleiro(jogo, mapa_serializado);

    Frame f = empacotar(TIPO_TABULEIRO, 0, mapa_serializado, TAM * TAM);
    envia_servidor(sock, "00:e0:4c:28:07:e3", (unsigned char*)&f, sizeof(Frame));
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
    jogo->tabuleiro[0][0]= JOGADOR;
    imprimir_tabuleiro(jogo);

    envia_tabuleiro(sock, jogo);

    while (1) {
        int lidos = recebe(sock, buffer, mac_origem);
        Frame f;
        if ((lidos > 0) && (protocolo_e_valido((char*)buffer, lidos))) {
            if (desempacotar(&f, buffer, lidos) == 0) {
                printf("Recebido tipo: %d de %s\n", f.tipo, mac_origem);
                if (f.tipo != 0) confirma_que_recebeu(sock, mac_origem, f);
                if (f.tipo == 10 || f.tipo == 11 || f.tipo == 12 || f.tipo ==13 ){
                    jogo->tabuleiro[jogo->jogador_x][jogo->jogador_y] = VISITADO;
                    switch (f.tipo) {
                        case 10: jogo->jogador_y ++; break;
                        case 11: jogo->jogador_x ++; break;
                        case 12: jogo->jogador_x --; break;
                        case 13: jogo->jogador_y --; break;
                    }
                    if (jogo->jogador_y < 0 ||  jogo->jogador_y >7 || jogo->jogador_x <0 || jogo->jogador_x >7){
                        if (jogo->jogador_y < 0) jogo->jogador_y = 0;
                        else if (jogo->jogador_y >7 ) jogo->jogador_y = 7;
                        else if (jogo->jogador_x <0) jogo->jogador_x = 0;
                        else if (jogo->jogador_x > 7) jogo->jogador_x = 7;
                        envia_nack(sock, mac_origem, f);
                    }
                    else if (jogo->tabuleiro[jogo->jogador_x][jogo->jogador_y] == TESOURO){
                        char* mensagem = "Você achou um tesouro!";
                        enviar_mensagem_texto(sock, mac_origem, mensagem);
                        jogo->tabuleiro[jogo->jogador_x][jogo->jogador_y] = JOGADOR;
                        envia_tabuleiro(sock, jogo);
                    }
                    else {
                        jogo->tabuleiro[jogo->jogador_x][jogo->jogador_y] = JOGADOR;
                        imprimir_tabuleiro(jogo);
                        envia_tabuleiro(sock, jogo);
                    }
                }
            }
             else {
                printf("Erro ao desempacotar frame.\n");
            }
        }
        
    }

    return 0;
}
