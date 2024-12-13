#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Definições de constantes
#define MAX_ADDRESS_BITS 32
#define READ 'R'
#define WRITE 'W'

// Estrutura para representar uma entrada na tabela de páginas
typedef struct PageTableEntry {
    int frame;          // Quadro físico associado (se presente)
    int valid;          // Bit de validade
    int modified;       // Bit de modificação
    int referenced;     // Bit de referência
    unsigned timestamp; // Timestamp para algoritmos como LRU
} PageTableEntry;

// Estrutura para representar a tabela de páginas hierárquica
typedef struct PageTableLevel {
    PageTableEntry **entries; // Ponteiro para entradas
    unsigned size;            // Número de entradas neste nível
} PageTableLevel;

// Variáveis globais
unsigned page_offset_bits;      // Bits do offset da página
unsigned level1_bits, level2_bits; // Bits para os níveis 1 e 2 da tabela
PageTableLevel *level1_table;  // Ponteiro para o nível 1 da tabela
unsigned memory_size_kb;       // Tamanho da memória física (em KB)
unsigned page_size_kb;         // Tamanho das páginas (em KB)
char replacement_policy[10];   // Política de substituição ("lru", "fifo", etc.)
long unsigned total_accesses = 0;   // Total de acessos
long unsigned page_faults = 0;      // Total de page faults
unsigned pages_written = 0;    // Total de páginas escritas (sujas)

// Estrutura para representar os quadros de memória
typedef struct Frame {
    int page_number;  // Número da página alocada no quadro
    int valid;        // Se o quadro está válido
    int modified;     // Se o quadro está modificado
    int referenced;   // Bit de referência para segunda chance
    unsigned last_access; // Timestamp para LRU
} Frame;

Frame *physical_memory; // Tabela invertida de quadros de memória
unsigned num_frames;   // Número de quadros na memória
unsigned current_time = 0; // Contador global de tempo para LRU

// Funções auxiliares
unsigned calculate_offset_bits(unsigned page_size_kb) {
    unsigned tmp = page_size_kb; // Tamanho da página em bytes
    unsigned s = 0;
    while (tmp > 1) {
        tmp >>= 1;
        s++;
    }
    return s;
}

unsigned calculate_level_bits(unsigned total_bits, unsigned offset_bits) {
    return (total_bits - offset_bits) / 2;
}

void initialize_page_table() {
    // Inicializar tabela de nível 1
    level1_table = (PageTableLevel *)malloc(sizeof(PageTableLevel));
    level1_table->size = (1 << level1_bits);
    level1_table->entries = (PageTableEntry **)calloc(level1_table->size, sizeof(PageTableEntry *));

    // Inicializar tabela invertida
    num_frames = memory_size_kb / page_size_kb;
    physical_memory = (Frame *)calloc(num_frames, sizeof(Frame));
}

// OLHAR OS BITS AQUI!!! Acho que está fazendo o mesmo

PageTableEntry *get_or_create_page_entry(unsigned virtual_address) {
    // Calcular índices nos níveis
    unsigned level1_index = (virtual_address >> level2_bits) & ((1 << level1_bits) - 1);
    unsigned level2_index = virtual_address & ((1 << level2_bits) - 1);
    //printf("Address: %d\n",virtual_address);
    // printf("level1_index: %d\n", level1_index);
    // printf("level2_index: %d\n", level2_index);

    // Acessar/Inicializar nível 2
    if (level1_table->entries[level1_index] == NULL) {
        level1_table->entries[level1_index] = (PageTableEntry *)calloc((1 << level2_bits), sizeof(PageTableEntry));
    }

    return &level1_table->entries[level1_index][level2_index];
}

int choose_frame_to_replace() {
    if (strcmp(replacement_policy, "lru") == 0) {
        
        unsigned lru_frame = 0;
        unsigned oldest_time = physical_memory[0].last_access;
        for (unsigned i = 1; i < num_frames; i++) {
            if (physical_memory[i].last_access < oldest_time) {
                oldest_time = physical_memory[i].last_access;
                lru_frame = i;
            }
        }
        return lru_frame;
        
    } else if (strcmp(replacement_policy, "fifo") == 0) {
        static int next_frame = 0;
        int victim = next_frame;
        next_frame = (next_frame + 1) % num_frames;
        return victim;
    } else if (strcmp(replacement_policy, "random") == 0) {
        return rand() % num_frames;
    } else if (strcmp(replacement_policy, "2a") == 0) {
        static int pointer = 0; // Ponteiro circular
        while (1) {
            int victim = pointer;
            pointer = (pointer + 1) % num_frames;

            // Verificar o bit de referência
            if (physical_memory[victim].referenced == 0) {
                return victim; // Página sem segunda chance, substitua
            } else {
                // Dar segunda chance e resetar o bit de referência
                physical_memory[victim].referenced = 0;
            }
        }
    } else {
        fprintf(stderr, "Algoritmo de substituição desconhecido: %s\n", replacement_policy);
        exit(1);
    }
    return 0;
}

void handle_page_fault(PageTableEntry *entry, unsigned virtual_address) {
    // Escolher quadro para substituir
    int frame_to_replace = choose_frame_to_replace();

    if (physical_memory[frame_to_replace].valid) {
        unsigned old_virtual_page = physical_memory[frame_to_replace].page_number;
        PageTableEntry *old_entry = get_or_create_page_entry(old_virtual_page);
        old_entry->valid = 0;
    }
    // Se o quadro a ser substituído está modificado, conte como "escrita"
    if (physical_memory[frame_to_replace].valid && physical_memory[frame_to_replace].modified) {
        pages_written++;
    }

    // Atualizar tabela invertida
    physical_memory[frame_to_replace].page_number = virtual_address;
    //printf("Page number: %d\n", virtual_address);
    physical_memory[frame_to_replace].valid = 1;
    physical_memory[frame_to_replace].modified = 0;
    physical_memory[frame_to_replace].referenced = 1;
    physical_memory[frame_to_replace].last_access = current_time;

    // Atualizar entrada da tabela de páginas
    entry->frame = frame_to_replace;
    entry->valid = 1;
}

void print_inverted_table() {
    printf("Tabela Invertida:\n");
    printf("-------------------------------------------------\n");
    printf("| Quadro | Página Virtual | Suja | Referenciada |\n");
    printf("-------------------------------------------------\n");
    for (unsigned i = 0; i < num_frames; i++) {
        printf("| %-6u | %-14d | %-4d | %-12d |\n",
               i,
               physical_memory[i].page_number,
               physical_memory[i].modified,
               physical_memory[i].referenced);
    }
    printf("-------------------------------------------------\n");
}

// Processamento do arquivo de entrada
void process_memory_access(FILE *file) {
    unsigned address;
    char access_type;

    while (fscanf(file, "%x %c", &address, &access_type) != EOF) {
        total_accesses++;
        current_time++;
        //printf("Offset bits = %d\n", page_offset_bits);
        address = address >> page_offset_bits;

        PageTableEntry *entry = get_or_create_page_entry(address);
        //print_inverted_table();
        if (!entry->valid) {
            // Page fault
            page_faults++;
            handle_page_fault(entry, address);
        }

        // Atualizar bits de controle
        Frame *frame = &physical_memory[entry->frame];
        frame->referenced = 1;
        frame->last_access = current_time;
        if (access_type == WRITE) {
            frame->modified = 1;
        }
    }
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <politica> <arquivo.log> <tamanho_pagina_kb> <tamanho_memoria_kb>\n", argv[0]);
        return 1;
    }

    // Configurações iniciais
    strncpy(replacement_policy, argv[1], sizeof(replacement_policy));
    const char *log_file = argv[2];
    page_size_kb = atoi(argv[3]) * 1024;
    memory_size_kb = atoi(argv[4]) * 1024;

    page_offset_bits = calculate_offset_bits(page_size_kb);

    level1_bits = calculate_level_bits(MAX_ADDRESS_BITS, page_offset_bits);
    level2_bits = MAX_ADDRESS_BITS - page_offset_bits - level1_bits;

    initialize_page_table();

    // Processar o arquivo de log
    FILE *file = fopen(log_file, "r");
    if (!file) {
        perror("Erro ao abrir arquivo de log");
        return 1;
    }

    process_memory_access(file);
    fclose(file);

    // Relatório final
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", log_file);
    printf("Tamanho da memoria: %u KB\n", memory_size_kb / 1024);
    printf("Tamanho das paginas: %u KB\n", page_size_kb / 1024);
    printf("Tecnica de reposicao: %s\n", replacement_policy);
    printf("Paginas lidas: %lu\n", page_faults);
    printf("Paginas escritas: %u\n", pages_written);
    printf("Total de acessos à memória: %lu\n", total_accesses);

    return 0;
}
