#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Estrutura para representar um quadro na tabela invertida
typedef struct {
    unsigned virtual_page; // Endereço virtual da página
    int dirty;             // Bit para indicar se a página foi alterada (1 = suja, 0 = limpa)
    int referenced;        // Bit para indicar se a página foi referenciada recentemente
    unsigned last_access;  // Tempo do último acesso (para LRU)
} Frame;

// Variáveis globais
Frame *inverted_table = NULL; // Tabela invertida
unsigned num_frames = 0;      // Número de quadros na memória física
unsigned page_size = 0;       // Tamanho de cada página (em KB)
unsigned mem_size = 0;        // Tamanho total da memória física (em KB)
char replacement_algo[10];    // Algoritmo de substituição (lru, fifo, random, etc.)
long unsigned access_count = 0;    // Contador de acessos à memória
unsigned page_faults = 0;     // Contador de page faults
unsigned dirty_pages_written = 0; // Contador de páginas "sujas" escritas no disco

// Protótipos das funções
void init_simulation();
void process_memory_access(FILE *file);
int find_page(unsigned virtual_page);
int choose_frame_to_replace();
void print_report();

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <algoritmo> <arquivo.log> <tamanho_pagina> <memoria_fisica>\n", argv[0]);
        return 1;
    }

    // Configurar os parâmetros do simulador
    strncpy(replacement_algo, argv[1], sizeof(replacement_algo) - 1);
    page_size = atoi(argv[3]) * 1024;
    mem_size = atoi(argv[4]) * 1024;
    num_frames = mem_size / page_size;

    // Inicializar tabela invertida
    inverted_table = (Frame *)calloc(num_frames, sizeof(Frame));
    if (!inverted_table) {
        fprintf(stderr, "Erro ao alocar memória para a tabela invertida.\n");
        return 1;
    }

    // Abrir o arquivo de entrada
    FILE *file = fopen(argv[2], "r");
    if (!file) {
        fprintf(stderr, "Erro ao abrir o arquivo %s.\n", argv[2]);
        free(inverted_table);
        return 1;
    }

    // Inicializar o simulador e processar acessos
    init_simulation();
    process_memory_access(file);

    // Fechar o arquivo e liberar memória
    fclose(file);
    print_report(argv[2]);
    free(inverted_table);

    return 0;
}

void init_simulation() {
    for (unsigned i = 0; i < num_frames; i++) {
        inverted_table[i].virtual_page = -1; // -1 indica quadro vazio
        inverted_table[i].dirty = 0;
        inverted_table[i].referenced = 0;
        inverted_table[i].last_access = 0;
    }
}

void process_memory_access(FILE *file) {
    unsigned addr;
    char rw;
    unsigned s = 0, tmp = page_size;
    // Calcular o número de bits para o deslocamento (s)
    while (tmp > 1) {
        tmp >>= 1;
        s++;
    }

    while (fscanf(file, "%x %c", &addr, &rw) == 2) {
        access_count++;
        unsigned virtual_page = addr >> s;

        int frame = find_page(virtual_page);
        if (frame == -1) {
            // Page fault
            page_faults++;
            frame = choose_frame_to_replace();

            // Escrever página suja de volta para o disco, se necessário
            if (inverted_table[frame].dirty) {
                dirty_pages_written++;
            }

            // Carregar nova página
            inverted_table[frame].virtual_page = virtual_page;
            inverted_table[frame].dirty = 0;
        }

        // Atualizar bits de controle
        inverted_table[frame].referenced = 1;
        inverted_table[frame].last_access = access_count;
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
    return -1; // Página não encontrada
}

int choose_frame_to_replace() {

    for (unsigned i = 0; i < num_frames; i++) {
        if (inverted_table[i].virtual_page == -1) {
            return i; // Retornar o índice do quadro vazio
        }
    }

    // Implementar os algoritmos de substituição de página
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
        static int pointer = 0; // Ponteiro circular
        while (1) {
            int victim = pointer;
            pointer = (pointer + 1) % num_frames;

            // Verificar o bit de referência
            if (inverted_table[victim].referenced == 0) {
                return victim; // Página sem segunda chance, substitua
            } else {
                // Dar segunda chance e resetar o bit de referência
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
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", input_file);
    printf("Tamanho da memoria: %u KB\n", mem_size / 1024);
    printf("Tamanho das paginas: %u KB\n", page_size / 1024);
    printf("Tecnica de reposicao: %s\n", replacement_algo);
    printf("Paginas lidas: %u\n", page_faults);
    printf("Paginas escritas: %u\n", dirty_pages_written);
    printf("Total de acessos à memória: %lu\n", access_count);
}
