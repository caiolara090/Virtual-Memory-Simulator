#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Estrutura para representar um quadro na tabela invertida
typedef struct {
    unsigned virtual_page;
    int dirty;
    int referenced;
    unsigned last_access;
    int valid;
} Frame;

// Variáveis globais
Frame *inverted_table = NULL;
unsigned num_frames = 0;
unsigned page_size = 0;
unsigned mem_size = 0;
char replacement_algo[10];
long unsigned access_count = 0;
unsigned page_faults = 0;
unsigned dirty_pages_written = 0;

// Funções auxiliares
void init_simulation();
void process_memory_access(FILE *file);
int find_page(unsigned virtual_page);
int choose_frame_to_replace();
void print_report();

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <algoritmo> <arquivo.log> <tamanho_pagina> <memoria_fisica>\n", argv[0]);
        return 1;
    }

    strncpy(replacement_algo, argv[1], sizeof(replacement_algo) - 1);
    page_size = atoi(argv[3]) * 1024;
    mem_size = atoi(argv[4]) * 1024;
    num_frames = mem_size / page_size;

    inverted_table = (Frame *)calloc(num_frames, sizeof(Frame));
    if (!inverted_table) {
        fprintf(stderr, "Erro ao alocar memória para a tabela invertida.\n");
        return 1;
    }

    FILE *file = fopen(argv[2], "r");
    if (!file) {
        fprintf(stderr, "Erro ao abrir o arquivo %s.\n", argv[2]);
        free(inverted_table);
        return 1;
    }

    init_simulation();
    process_memory_access(file);

    fclose(file);
    print_report(argv[2]);
    free(inverted_table);

    return 0;
}

// Inicializar a memória física e tabela de páginas
void init_simulation() {
    for (unsigned i = 0; i < num_frames; i++) {
        inverted_table[i].virtual_page = -1;
        inverted_table[i].dirty = 0;
        inverted_table[i].referenced = 0;
        inverted_table[i].last_access = 0;
        inverted_table[i].valid = 0;
    }
}

//Simula a execução de um acesso à memória com uma dada função (leitura ou escrita)
void process_memory_access(FILE *file) {
    unsigned addr;
    char rw;
    unsigned s = 0, tmp = page_size;

    while (tmp > 1) {
        tmp >>= 1;
        s++;
    }

    while (fscanf(file, "%x %c", &addr, &rw) == 2) {
        access_count++;
        unsigned virtual_page = addr >> s;

        int frame = find_page(virtual_page);
        if (frame == -1) {

            page_faults++;
            frame = choose_frame_to_replace();

            if (inverted_table[frame].dirty) {
                dirty_pages_written++;
            }

            inverted_table[frame].virtual_page = virtual_page;
            inverted_table[frame].dirty = 0;
        } else {

        inverted_table[frame].referenced = 1;
        inverted_table[frame].last_access = access_count;
        
        }

        if (rw == 'W') {
            inverted_table[frame].dirty = 1;
        }
    }
}

int find_page(unsigned virtual_page) {
    for (unsigned i = 0; i < num_frames; i++) {
        if (inverted_table[i].virtual_page == virtual_page) {
            return i;
        }
    }
    return -1;
}

int choose_frame_to_replace() {

    for (unsigned i = 0; i < num_frames; i++) {
        if (inverted_table[i].virtual_page == -1) {
            return i;
        }
    }

    // Implementação dos algoritmos de substituição de página
    if (strcmp(replacement_algo, "lru") == 0) {
        unsigned lru_frame = 0;
        unsigned oldest_time = inverted_table[0].last_access;
        for (unsigned i = 1; i < num_frames; i++) {
            if (inverted_table[i].last_access < oldest_time) {
                oldest_time = inverted_table[i].last_access;
                lru_frame = i;
            }
        }
        return lru_frame;

    } else if (strcmp(replacement_algo, "fifo") == 0) {

        static int next_frame = 0;
        int victim = next_frame;
        next_frame = (next_frame + 1) % num_frames;
        return victim;

    } else if (strcmp(replacement_algo, "random") == 0) {

        return random() % num_frames;

    } else if (strcmp(replacement_algo, "2a") == 0) { 

        static int pointer = 0;
        while (1) {
            int victim = pointer;
            pointer = (pointer + 1) % num_frames;

            if (inverted_table[victim].referenced == 0) {
                return victim; 
            } else {
                inverted_table[victim].referenced = 0;
            }
        }
    } else {
        fprintf(stderr, "Algoritmo de substituição desconhecido: %s\n", replacement_algo);
        exit(1);
    }
    return 0;
}

void print_report(const char *input_file) {

    // printf("Memória gasta = %.2f KB\n", (double)(num_frames) / 128.0);
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", input_file);
    printf("Tamanho da memoria: %u KB\n", mem_size / 1024);
    printf("Tamanho das paginas: %u KB\n", page_size / 1024);
    printf("Tecnica de reposicao: %s\n", replacement_algo);
    printf("Paginas lidas: %u\n", page_faults);
    printf("Paginas escritas: %u\n", dirty_pages_written);
    printf("Total de acessos à memória: %lu\n", access_count);
}
