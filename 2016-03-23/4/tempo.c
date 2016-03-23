#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"
/* Eventos */
#define TEST 1
#define FAULT 2
#define REPAIR 3

#define UNKNOWN 0
#define FAULTY 1
#define FAULT_FREE 2

/* Node descriptor */
typedef struct {
	int id; // SMPL facility identifier
	int status;
	int *state;
	// Other local structures are defined here
} Node;

Node *nodes;

char* status_string(int s) {
	if (s == UNKNOWN) {
		return "UNKNOWN";
	}
	if (s == FAULTY) {
		return "FAULTY";
 	}
 	if (s == FAULT_FREE) {
		return "FAULT-FREE";
	}
	//printf("%d", s);
	return "";
}

void print_state(Node n, int N) {
	int i;
	for (i=0; i<N; i++) {
		printf("%12d", i);
	}
	printf("\n");
	for (i=0; i<N; i++) {
		printf("%12s", status_string(n.state[i]));
		//printf("%12d", n.state[i]);
	}
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
		nodes[i].status = FAULT_FREE;
		nodes[i].state = (int *) malloc(N*sizeof(int));
		memset (nodes[i].state, UNKNOWN, N*sizeof(int));
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
				if(status(nodes[token].id != 0)) {
					break;
				}
				printf("Sou o nodo %d, vou testar no tempo %5.1f\n", token, time());

				int j = token;
				do {
					j = (j+1) % N;

					printf("Estou testando o nodo %d, cujo status é: %s\n", j, status_string(nodes[j].status));
					
					nodes[token].state[j] = nodes[j].status;
					if (nodes[j].status == FAULT_FREE) {
						// Get J's state array
					}

				} while (nodes[j].status != FAULT_FREE);

				print_state(nodes[token], N);
				printf("\n\n");
				schedule(TEST, 30.0, token);
				break;
			case FAULT:
				r = request(nodes[token].id, token, 0);
				if (r != 0) {
					puts("Não consegui falhar o nodo!");
					exit(1);
				}
				printf("Sou o nodo %d, falhei no tempo %5.1f\n", token, time());

				nodes[token].status = FAULTY;

				break;
			case REPAIR:
				release(nodes[token].id, token);
				printf("Sou o nodo %d, recuperei no tempo %5.1f\n", token, time());

				nodes[token].status = FAULT_FREE;

				schedule(TEST, 30.0, token);
				break;
		}
	}
}