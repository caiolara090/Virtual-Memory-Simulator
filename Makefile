# Variáveis
CC = gcc
CFLAGS = -Wall -g
SOURCES = tp2virtual.c doisNiveis.c tresNiveis.c inverted.c dense.c
OBJECTS = $(SOURCES:.c=.o)
TARGETS = tp2virtual doisNiveis tresNiveis inverted dense

# Regra principal
all: $(TARGETS)

# Regra para compilar cada executável
tp2virtual: tp2virtual.o
	$(CC) $(CFLAGS) -o tp2virtual tp2virtual.o

dense: dense.o
	$(CC) $(CFLAGS) -o dense dense.o

doisNiveis: doisNiveis.o
	$(CC) $(CFLAGS) -o doisNiveis doisNiveis.o

tresNiveis: tresNiveis.o
	$(CC) $(CFLAGS) -o tresNiveis tresNiveis.o

inverted: inverted.o
	$(CC) $(CFLAGS) -o inverted inverted.o

# Regra genérica para compilar os arquivos .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(OBJECTS) $(TARGETS)

.PHONY: all clean
