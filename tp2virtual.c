#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

// Esse código é o programa principal, que direciona a execução de acordo com os parâmetros passados pelo usuário
// Veja que ele chama os outros programas como se o usuário fizesse uma chamada pela linha de comando
int main(int argc, char *argv[]) {

    // struct timeval start, end;
    // gettimeofday(&start, NULL);

    if (argc != 6) {
        fprintf(stderr, "Uso: tp2virtual <algoritmo> <arquivo.log> <tamanho_pagina_kb> <memoria_kb> <tipo_tabela>\n\nAs tabelas podem ser do tipo: dense, doisNiveis, tresNiveis ou inverted\n");
        exit(EXIT_FAILURE);
    }

    // Inicialização
    const char *table_type = argv[5];

    char command[256];
    const char *arg1 = argv[1];
    const char *arg2 = argv[2];
    const char *arg3 = argv[3];
    const char *arg4 = argv[4];


    int tamanho_quadro = atoi(argv[3]);
    int tamanho_memoria = atoi(argv[4]);

    if (tamanho_quadro < 2 || tamanho_quadro > 64){
        fprintf(stderr, "Tamanho de quadro %d fora dos limites de 2 KB a 64 KB\n", tamanho_quadro);
        exit(EXIT_FAILURE);
    }

    if (tamanho_memoria < 128 || tamanho_memoria > 16384){
        fprintf(stderr, "Tamanho de memoria %d fora dos limites de 128 KB a 16 MB\n", tamanho_quadro);
        exit(EXIT_FAILURE);
    }

    if (strcmp(table_type, "dense") == 0) {

        sprintf(command, "./dense %s %s %s %s", arg1, arg2, arg3, arg4);

    } else if (strcmp(table_type, "doisNiveis") == 0) {

        sprintf(command, "./doisNiveis %s %s %s %s", arg1, arg2, arg3, arg4);

    } else if (strcmp(table_type, "tresNiveis") == 0) {

        sprintf(command, "./tresNiveis %s %s %s %s", arg1, arg2, arg3, arg4);

    } else if (strcmp(table_type, "inverted") == 0) {

        sprintf(command, "./inverted %s %s %s %s", arg1, arg2, arg3, arg4);

    } else {
        printf("Escolha uma tabela da lista: dense, doisNiveis, tresNiveis ou inverted\n\t\t\t : ( \n");
    }

    // printf("%s %s %s %s %s\n", arg1, table_type, arg2, arg3, arg4);
    system(command);

    // gettimeofday(&end, NULL);
    // long seconds = end.tv_sec - start.tv_sec;
    // long microseconds = end.tv_usec - start.tv_usec;
    // long total_time = seconds * 1000000 + microseconds;
    // printf("Tempo decorrido: %lf milisegundos\n\n", total_time/1000.0);

    return 0;
}