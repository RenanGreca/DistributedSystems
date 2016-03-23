#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"
/* Eventos */
#define TEST 1
#define FAULT 2
#define REPAIR 3

/* descritor do nodo */
typedef struct {
	int id; // Identificador de facility SMPL
	// Outras estruturas locais são definidas aqui.
} Nodo;

Nodo *nodos;

/* Corpo do programa */
int main(int argc, char *argv[]) {
	static int N; // Número de nodos do sistema
	static int token;
	static int event;
	static int r;
	static int i;
	static char fa_name[5];
	
	if (argc != 2) {
		puts("Uso correto: tempo [num-nodos]");
		exit(1);
	}
	
	N = atoi(argv[1]);
	smpl(0, "programa tempo");
	reset();
	stream(1); // 1 thread de execução
	
	// Inicialização dos nodos
	nodos = (Nodo*) malloc(N*sizeof(Nodo));
	for (i=0; i<N; i++) {
		memset (fa_name, '\0', 5);
		sprintf(fa_name, "%d", i);
		nodos[i].id = facility(fa_name, 1);
	}
	
	for (i=0; i<N; i++) {
		schedule (TEST, 30.0, i);
	}
	schedule (FAULT, 31.0, 2);
	schedule (REPAIR, 61.0, 2);
	
	while (time() < 100.0) {
		cause(&event, &token);
		switch(event) {
			case TEST:
				if(status(nodos[token].id != 0)) {
					break;
				}
				printf("Sou o nodo %d, vou testar no tempo %5.1f\n", token, time());
				schedule(TEST, 30.0, token);
				break;
			case FAULT:
				r = request(nodos[token].id, token, 0);
				if (r != 0) {
					puts("Não consegui falhar o nodo!");
					exit(1);
				}
				printf("Sou o nodo %d, falhei no tempo %5.1f\n", token, time());
				break;
			case REPAIR:
				release(nodos[token].id, token);
				printf("Sou o nodo %d, recuperei no tempo %5.1f\n", token, time());
				schedule(TEST, 30.0, token);
				break;
		}
	}
} 
