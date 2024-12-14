#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Constantes globais
#define MAX_ADDRESS_BITS 32
#define READ 'R'
#define WRITE 'W'

// Estruturas de dados
typedef struct PageTableEntry {
    int frame;          
    int valid;         
    int modified;       
    int referenced;    
    unsigned timestamp;
} PageTableEntry;


typedef struct PageTableLevel {
    void **entries;
    unsigned size;
} PageTableLevel;

// Variáveis globais
unsigned page_offset_bits;        
unsigned level1_bits, level2_bits, level3_bits;
PageTableLevel *level1_table;   
unsigned memory_size_kb;          
unsigned page_size_kb;             
char replacement_policy[10];     
long unsigned total_accesses = 0;    
long unsigned page_faults = 0;          
long unsigned pages_written = 0;     

// Estrutura para representar os quadros de memória
typedef struct Frame {
    int page_number;
    int valid;       
    int modified;   
    int referenced;   
    unsigned last_access;
} Frame;

Frame *physical_memory;
unsigned num_frames;
unsigned current_time = 0;

// Funções auxiliares
unsigned calculate_offset_bits(unsigned page_size_kb) {
    unsigned tmp = page_size_kb;
    unsigned s = 0;
    while (tmp > 1) {
        tmp >>= 1;
        s++;
    }
    return s;
}

// Calcula os bits de offset para 3 hierarquias
unsigned calculate_level_bits(unsigned total_bits, unsigned offset_bits) {
    return (total_bits - offset_bits) / 3;
}

// Inicializar a memória física e tabela de páginas
void initialize_page_table() {

    level1_table = (PageTableLevel *)malloc(sizeof(PageTableLevel));
    level1_table->size = (1 << level1_bits);
    level1_table->entries = (void **)calloc(level1_table->size, sizeof(void *));

    num_frames = memory_size_kb / page_size_kb;
    physical_memory = (Frame *)calloc(num_frames, sizeof(Frame));
}

PageTableEntry *get_or_create_page_entry(unsigned virtual_address) {

    unsigned level1_index = (virtual_address >> (level2_bits + level3_bits)) & ((1 << level1_bits) - 1);
    unsigned level2_index = (virtual_address >> level3_bits) & ((1 << level2_bits) - 1);
    unsigned level3_index = virtual_address & ((1 << level3_bits) - 1);

    if (level1_table->entries[level1_index] == NULL) {
        level1_table->entries[level1_index] = (PageTableLevel *)malloc(sizeof(PageTableLevel));
        PageTableLevel *level2_table = (PageTableLevel *)level1_table->entries[level1_index];
        level2_table->size = (1 << level2_bits);
        level2_table->entries = (void **)calloc(level2_table->size, sizeof(void *));
    }
    PageTableLevel *level2_table = (PageTableLevel *)level1_table->entries[level1_index];

    if (level2_table->entries[level2_index] == NULL) {
        level2_table->entries[level2_index] = (PageTableEntry *)calloc((1 << level3_bits), sizeof(PageTableEntry));
    }
    PageTableEntry *level3_table = (PageTableEntry *)level2_table->entries[level2_index];

    return &level3_table[level3_index];
}

// Algoritmos de seleção de página a ser retirada da memória
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
        static int pointer = 0;
        while (1) {
            int victim = pointer;
            pointer = (pointer + 1) % num_frames;

            if (physical_memory[victim].referenced == 0) {
                return victim;
            } else {
                physical_memory[victim].referenced = 0;
            }
        }
    } else {
        fprintf(stderr, "Algoritmo de substituição desconhecido: %s\n", replacement_policy);
        exit(1);
    }
    return 0;
}

//Lida com a falta de uma página na memória
void handle_page_fault(PageTableEntry *entry, unsigned virtual_address) {

    int frame_to_replace = choose_frame_to_replace();

    if (physical_memory[frame_to_replace].valid) {
        unsigned old_virtual_page = physical_memory[frame_to_replace].page_number;
        PageTableEntry *old_entry = get_or_create_page_entry(old_virtual_page);
        old_entry->valid = 0;
    }

    if (physical_memory[frame_to_replace].valid && physical_memory[frame_to_replace].modified) {
        pages_written++;
    }

    physical_memory[frame_to_replace].page_number = virtual_address;
    physical_memory[frame_to_replace].valid = 1;
    physical_memory[frame_to_replace].modified = 0;

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

        address = address >> page_offset_bits;
        total_accesses++;
        current_time++;

        PageTableEntry *entry = get_or_create_page_entry(address);
        if (!entry->valid) {

            page_faults++;
            handle_page_fault(entry, address);
        } else {

        Frame *frame = &physical_memory[entry->frame];
        frame->referenced = 1;
        frame->last_access = current_time;
        }

        Frame *frame = &physical_memory[entry->frame];
        if (access_type == WRITE) {
            frame->modified = 1;
        }
    }
}

// Função para calcular o tamanho da tabela - usada nos testes e no relatório
void calculate_table_size() {
    unsigned total_entries_level1_used = 0;
    unsigned total_entries_level2_used = 0;
    unsigned total_entries_level3_used = 0;

    for (unsigned i = 0; i < level1_table->size; i++) {
        if (level1_table->entries[i] != NULL) {
            total_entries_level1_used++;

            PageTableLevel *level2_table = (PageTableLevel *)level1_table->entries[i];
            for (unsigned j = 0; j < level2_table->size; j++) {
                if (level2_table->entries[j] != NULL) {
                    total_entries_level2_used++;

                    PageTableEntry *level3_table = (PageTableEntry *)level2_table->entries[j];
                    unsigned level3_count = 0;
                    for (unsigned k = 0; k < (1 << level3_bits); k++) {
                        if (level3_table[k].valid) {
                            level3_count++;
                        }
                    }

                    if (level3_count > 0) {
                        total_entries_level3_used++;
                    }
                }
            }
        }
    }

    unsigned memory_used_kb = 8 * (
        level1_table->size +
        total_entries_level2_used * (1 << level2_bits) +
        total_entries_level3_used * (1 << level3_bits)
    ) / 1024;

    printf("Memória gasta = %d KB\n", memory_used_kb);
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <politica> <arquivo.log> <tamanho_pagina_kb> <tamanho_memoria_kb>\n", argv[0]);
        return 1;
    }

    strncpy(replacement_policy, argv[1], sizeof(replacement_policy));
    const char *log_file = argv[2];
    page_size_kb = atoi(argv[3]) * 1024;
    memory_size_kb = atoi(argv[4]) * 1024;

    page_offset_bits = calculate_offset_bits(page_size_kb);
    level1_bits = calculate_level_bits(MAX_ADDRESS_BITS, page_offset_bits);
    level2_bits = calculate_level_bits(MAX_ADDRESS_BITS, page_offset_bits);
    level3_bits = MAX_ADDRESS_BITS - page_offset_bits - level1_bits - level2_bits;

    initialize_page_table();

    FILE *file = fopen(log_file, "r");
    if (!file) {
        perror("Erro ao abrir arquivo de log");
        return 1;
    }

    process_memory_access(file);
    fclose(file);

    //calculate_table_size();
    //Relatório final
    
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", log_file);
    printf("Tamanho da memoria: %u KB\n", memory_size_kb / 1024);
    printf("Tamanho das paginas: %u KB\n", page_size_kb / 1024);
    printf("Tecnica de reposicao: %s\n", replacement_policy);
    printf("Paginas lidas: %lu\n", page_faults);
    printf("Paginas escritas: %lu\n", pages_written);
    printf("Total de acessos à memória: %lu\n", total_accesses);

    return 0;
}
