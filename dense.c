#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Constantes globais
#define MAX_PAGE_TABLE_SIZE (1 << 21) // Máximo número de páginas (para páginas de 2 KB)
#define TRUE 1
#define FALSE 0

// Estruturas de dados
typedef struct {
    int page_number;
    int frame_number;
    int valid;
    int modified;
    unsigned long last_access_time; // Para LRU
} PageTableEntry;

typedef struct {
    int frame_number;
    int page_number;
    int valid;
    int modified;
} Frame;

// Variáveis globais
unsigned page_size, memory_size;
unsigned num_frames;
unsigned s; // Quantidade de bits para deslocamento
Frame *physical_memory;
PageTableEntry *page_table;
char replacement_policy[10];
unsigned long access_count = 0;
unsigned long page_faults = 0;
unsigned long dirty_pages_written = 0;

// Funções auxiliares
void parse_arguments(int argc, char *argv[]);
void initialize_simulator();
void process_memory_access(unsigned addr, char rw);
int find_page_in_memory(int page_number);
void handle_page_fault(int page_number, char rw);
int select_victim_frame();
void print_report(const char *input_file);
int iteracao = 0;

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: tp2virtual <algoritmo> <arquivo.log> <tamanho_pagina_kb> <memoria_kb>\n");
        exit(EXIT_FAILURE);
    }

    // Parse dos argumentos
    parse_arguments(argc, argv);

    // Inicialização
    initialize_simulator();

    // Leitura do arquivo de entrada
    FILE *input_file = fopen(argv[2], "r");
    if (!input_file) {
        perror("Erro ao abrir o arquivo de entrada");
        exit(EXIT_FAILURE);
    }

    unsigned addr;
    char rw;
    while (fscanf(input_file, "%x %c", &addr, &rw) != EOF) {
        process_memory_access(addr, rw);
    }
    fclose(input_file);

    // Relatório final
    print_report(argv[2]);

    // Liberar memória -  ESTÁ COM VAZAMENTO!!
    //free(physical_memory);
    //free(page_table);

    return 0;
}

void parse_arguments(int argc, char *argv[]) {

    strcpy(replacement_policy, argv[1]);

    if (strcmp(replacement_policy, "random") != 0 &&
    strcmp(replacement_policy, "fifo") != 0 &&
    strcmp(replacement_policy, "lru") != 0 &&
    strcmp(replacement_policy, "2a") != 0) {
    fprintf(stderr, "Erro: Política de substituição desconhecida: %s\n", replacement_policy);
    exit(EXIT_FAILURE);
}

    page_size = atoi(argv[3]) * 1024; // Converter KB para bytes
    memory_size = atoi(argv[4]) * 1024;
    num_frames = memory_size / page_size;

    // Calcular o deslocamento s
    unsigned tmp = page_size;
    s = 0;
    while (tmp > 1) {
        tmp >>= 1;
        s++;
    }
}

void initialize_simulator() {

    srandom((unsigned)time(NULL));
    // Inicializar a memória física e tabela de páginas
    physical_memory = (Frame *)malloc(num_frames * sizeof(Frame));
    page_table = (PageTableEntry *)malloc(MAX_PAGE_TABLE_SIZE * sizeof(PageTableEntry));

    if (!physical_memory || !page_table) {
        fprintf(stderr, "Erro ao alocar memória para o simulador\n");
        exit(EXIT_FAILURE);
    }

    // Inicializar todas as entradas como inválidas
    for (unsigned i = 0; i < num_frames; i++) {
        physical_memory[i].valid = FALSE;
        physical_memory[i].modified = FALSE;
        physical_memory[i].page_number = -1;
    }

    for (unsigned i = 0; i < MAX_PAGE_TABLE_SIZE; i++) {
        page_table[i].valid = FALSE;
    }
}

void process_memory_access(unsigned addr, char rw) {

    int page_number = addr >> s;
    access_count++;
    int frame_index = find_page_in_memory(page_number);

    if (frame_index == -1) {
        // Page fault
        page_faults++;
        handle_page_fault(page_number, rw);
    } else {
        // Atualizar bits da página na memória
        physical_memory[frame_index].valid = TRUE;
        if (rw == 'W') {
            physical_memory[frame_index].modified = TRUE;
        }
        page_table[page_number].last_access_time = access_count; // Para LRU
    }
}

int find_page_in_memory(int page_number) {
    for (unsigned i = 0; i < num_frames; i++) {
        if (physical_memory[i].valid && physical_memory[i].page_number == page_number) {
            return i;
        }
    }
    return -1;
}

void handle_page_fault(int page_number, char rw) {
    int victim_frame = select_victim_frame();

    if (physical_memory[victim_frame].valid) {
    page_table[physical_memory[victim_frame].page_number].valid = FALSE;
}

    if (physical_memory[victim_frame].valid && physical_memory[victim_frame].modified) {
        dirty_pages_written++;
    }

    // Substituir a página no quadro
    physical_memory[victim_frame].page_number = page_number;
    physical_memory[victim_frame].valid = TRUE;
    physical_memory[victim_frame].modified = (rw == 'W');

    page_table[page_number].frame_number = victim_frame;
    page_table[page_number].valid = TRUE;
    page_table[page_number].last_access_time = access_count;
}

int select_victim_frame() {

    for (unsigned i = 0; i < num_frames; i++) {
        if (!physical_memory[i].valid) {
            return i; // Retorna imediatamente o primeiro quadro livre encontrado.
        }
    }

    if (strcmp(replacement_policy, "random") == 0) {
        return random() % num_frames;
    } else if (strcmp(replacement_policy, "fifo") == 0) {
        static int next_frame = 0;
        int victim = next_frame;
        next_frame = (next_frame + 1) % num_frames;
        return victim;
    } else if (strcmp(replacement_policy, "lru") == 0) {
        unsigned long oldest_time = ~0UL; // Maior valor possível para comparar corretamente.
        int victim = -1;

        // Itera sobre todos os quadros para encontrar o menos recentemente usado.
        for (unsigned i = 0; i < num_frames; i++) {
            if (physical_memory[i].valid) { // Apenas considera quadros válidos.
                unsigned page_number = physical_memory[i].page_number;
                unsigned long last_access = page_table[page_number].last_access_time;

                if (last_access < oldest_time) {
                    oldest_time = last_access;
                    victim = i;
                }
            }
        }

        return victim;
        
    } else if (strcmp(replacement_policy, "2a") == 0) {
    static int pointer = 0; // Ponteiro circular
    while (TRUE) {
        int victim = pointer;
        pointer = (pointer + 1) % num_frames;

        // Verificar se tem segunda chance
        if (page_table[physical_memory[victim].page_number].last_access_time < access_count) {
            // Não tem segunda chance, selecionar
            return victim;
        } else {
            // Dar segunda chance e resetar o bit
            page_table[physical_memory[victim].page_number].last_access_time = 0;
        }
    }
}
    // Implementar outros algoritmos aqui (2a chance, etc.)
    return 0;
}

void print_report(const char *input_file) {
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", input_file);
    printf("Tamanho da memoria: %u KB\n", memory_size / 1024);
    printf("Tamanho das páginas: %u KB\n", page_size / 1024);
    printf("Tecnica de reposicao: %s\n", replacement_policy);
    printf("Paginas lidas: %lu\n", page_faults);
    printf("Paginas escritas: %lu\n", dirty_pages_written);
    printf("Total de acessos à memória: %lu\n", access_count);
}