#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"
/* Eventos */
#define TEST 1
#define FAULT 2
#define REPAIR 3

#define FAULT_FREE 0
#define FAULTY 1
#define UNKNOWN 2

/* Node descriptor */
typedef struct {
	int id; // SMPL facility identifier
	// Other local structures are defined here
} Node;

Node *nodes;

char* status_string(int s) {
	if (s == FAULTY) {
		return "FAULTY";
 	}
 	if (s == FAULT_FREE) {
		return "FAULT-FREE";
	}
	return "UNKNOWN";
}

/* Program body */
int main(int argc, char *argv[]) {
	static int N; // Number of nodes
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
	stream(1); // 1 execution thread
	
	// Node intialization
	nodes = (Node*) malloc(N*sizeof(Node));
	for (i=0; i<N; i++) {
		memset (fa_name, '\0', 5);
		sprintf(fa_name, "%d", i);
		nodes[i].id = facility(fa_name, 1);
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
				if(status(nodes[token].id) != FAULT_FREE) {
					break;
				}
				printf("Sou o nodo %d, vou testar no tempo %5.1f\n", token, time());

				int j = (token+1) % N;

				printf("Estou testando o nodo %d, cujo status é: %s\n", j, status_string(status(nodes[j].id)));
				printf("\n");
				schedule(TEST, 30.0, token);
				break;
			case FAULT:
				r = request(nodes[token].id, token, 0);
				if (r != 0) {
					puts("Não consegui falhar o nodo!");
					exit(1);
				}
				printf("Sou o nodo %d, falhei no tempo %5.1f\n", token, time());

				break;
			case REPAIR:
				release(nodes[token].id, token);
				printf("Sou o nodo %d, recuperei no tempo %5.1f\n", token, time());

				schedule(TEST, 30.0, token);
				break;
		}
	}
}