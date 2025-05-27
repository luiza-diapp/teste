#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define TAM 8

typedef enum {
    VAZIO,
    TESOURO,
    VISITADO,
    JOGADOR
} Estado;

typedef struct {
    Estado** tabuleiro;
    int jogador_x;
    int jogador_y;
} Jogo;

// Cria e inicializa a estrutura do jogo
Jogo* criar_jogo();

// Libera a memória alocada
void destruir_jogo(Jogo* jogo);

void inicializar_tabuleiro(Jogo* jogo);

void imprimir_tabuleiro(const Jogo* jogo);
