// protocolo.h
#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <stdint.h>

#define TAM_MAX_DADOS 127
#define MARCADOR_INICIO 0x7E  // 0111 1110

// Tipos de mensagem
enum TipoMensagem {
    TIPO_ACK = 0,
    TIPO_NACK = 1,
    TIPO_OK_ACK = 2,
    TIPO_LIVRE = 3,
    TIPO_TAMANHO = 4,
    TIPO_DADOS = 5,
    TIPO_TEXTO = 6,
    TIPO_VIDEO = 7,
    TIPO_IMAGEM = 8,
    TIPO_FIM_ARQUIVO = 9,
    TIPO_DIREITA = 10,
    TIPO_CIMA = 11,
    TIPO_BAIXO = 12,
    TIPO_ESQUERDA = 13,
    TIPO_LIVRE2 = 14,
    TIPO_ERRO = 15,
    TIPO_TABULEIRO =16
};

// Frame do protocolo
typedef struct {
    uint8_t marcador;            // 8 bits
    uint8_t tamanho;             // 7 bits (use inteiro de 8 bits)
    uint8_t sequencia;           // 5 bits (use inteiro de 8 bits)
    uint8_t tipo;                // 4 bits (use inteiro de 8 bits)
    uint8_t checksum;            // 8 bits
    uint8_t dados[TAM_MAX_DADOS]; // até 127 bytes
} Frame;

// Funções
Frame empacotar(uint8_t tipo, uint8_t sequencia, uint8_t* dados, uint8_t tamanho);
int desempacotar(Frame* f, const uint8_t* buffer, int buflen);
uint8_t calcular_checksum(const Frame* f);

#endif
