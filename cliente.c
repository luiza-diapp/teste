#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "protocolo.h"
#include "tabuleiroservidor.h"

extern int cria_raw_socket(const char* interface);
extern int envia(int socket, const char* destino, const unsigned char* dados, int tamanho);
extern int recebe(int socket, unsigned char* dados, char* origem);
extern int protocolo_e_valido(char* buffer, int tamanho_buffer);


int mostra_tabuleiro (char* buffer, Frame f){

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


void envia_movimento(char teclaescolhida, Frame f, int sock, char mac_origem[18]){
    uint8_t tipo_movimento;

    switch (teclaescolhida) {
        case 'w': tipo_movimento = TIPO_CIMA; break;
        case 's': tipo_movimento = TIPO_BAIXO; break;
        case 'd': tipo_movimento = TIPO_DIREITA; break;
        case 'a': tipo_movimento = TIPO_ESQUERDA; break;
        default:
            printf("Tecla inválida no envio.\n");
            return;
    }

    uint8_t vazio[] = {0};
    Frame movimento = empacotar(tipo_movimento, f.sequencia + 1, vazio, 0);
    envia(sock, mac_origem, (unsigned char*)&movimento, sizeof(Frame));
    printf("Movimento enviado (%d).\n", tipo_movimento);
}


int confirma_que_recebeu(int sock, char mac_origem[18], Frame f){
    uint8_t vazio[] = {0};
    Frame ack = empacotar(TIPO_ACK, f.sequencia, vazio, 0);

    // Enviar o ACK de volta para o MAC de origem
    envia(sock, mac_origem, (unsigned char*)&ack, sizeof(Frame));
    printf("ACK enviado para %s\n", mac_origem);
}

char ler_tecla_valida() {
    char c;
    while (1) {
        printf("Digite uma tecla (w/a/s/d): ");
        if (scanf(" %c", &c) == 1) {
            if (c == 'w' || c == 'a' || c == 's' || c == 'd') {
                return c;
            } else {
                printf("Tecla inválida! Use apenas w/a/s/d.\n");
                // limpa qualquer caractere extra, inclusive '\n'
                while (getchar() != '\n');
            }
        }
    }
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
        Frame f;
        if ((lidos > 0)  && (protocolo_e_valido((char*)buffer, lidos))) {
            if (desempacotar(&f, buffer, lidos) == 0) {
                printf("Recebido tipo: %d de %s\n", f.tipo, mac_origem);
                if (f.tipo != 0) confirma_que_recebeu(sock, mac_origem, f);
                if (f.tipo == 16 || f.tipo == 10 || f.tipo == 11 || f.tipo == 13 || f.tipo == 14){
                    mostra_tabuleiro(buffer, f);
                    printf("Para andar no mapa pressione alguma das teclas: ⬆, ⬇, ⮕, ⬅ \n");
                    char teclaescolhida = ler_tecla_valida();
                    envia_movimento(teclaescolhida, f, sock, mac_origem);
                }
                else if (f.tipo == 6){
                    receber_mensagem_texto(sock);
                }
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



