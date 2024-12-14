#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

int main(int argc, char *argv[]) {

    char command[800];

    char algoritmos[4][256];
    sprintf(algoritmos[0], "lru");
    sprintf(algoritmos[1], "2a");
    sprintf(algoritmos[2], "fifo");
    sprintf(algoritmos[3], "random");


    int valor_pagina[4] = {2, 16, 64};
    int valor_mem[4] = {256, 2048, 16384};

    char arquivos[4][256];

    sprintf(arquivos[0], "matriz/matriz.log");
    sprintf(arquivos[1], "compilador/compilador.log");
    sprintf(arquivos[2], "compressor/compressor.log");
    sprintf(arquivos[3], "simulador/simulador.log");

    char tabelas[4][256];

    sprintf(tabelas[0], "dense");
    sprintf(tabelas[1], "doisNiveis");
    sprintf(tabelas[2], "tresNiveis");
    sprintf(tabelas[3], "inverted");

    for (int i_tabelas = 0; i_tabelas < 4; i_tabelas++){
         for (int i_arq = 0; i_arq < 4; i_arq++){
            for (int i_alg = 0; i_alg < 4; i_alg++){
                 for (int i_mem = 0; i_mem < 3; i_mem++){
                     for (int i_pag = 0; i_pag < 3; i_pag++){
                                           
                        sprintf(command, "./tp2virtual %s %s %d %d %s", algoritmos[i_alg], arquivos[i_arq], valor_pagina[i_pag], valor_mem[i_mem], tabelas[i_tabelas]);

                        system(command);

                    }
                }
            }
        }
    }

    return 0;

}
