/*
 *  Distributed Mutual Exclusion with Ricart-Agrawala algorithm
 *  Developed by Renan Greca
 *  https://github.com/RenanGreca/DistributedSystems  
 *
 *  Developed for Distributed Systems class
 *  Prof. Elias P. Duarte Jr.
 *  Universidade Federal do Paraná; Curitiba, Brazil
 *  May 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include "smpl.h"
#include "rand.h"

/* Events */
#define REQUEST 0
#define RECEIVE_REQUEST 1
#define RECEIVE_REPLY 2
#define RELEASE 3

/* Time it takes to use the critical region (C.R.) */
#define RUNNING_INTERVAL 50.0
/* Maximum time it takes for a message to arrive */ 
#define MAX_MESSAGE_DELAY 20.0 

/* Color constants for pretty printing */
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define RESET "\033[0m"

/* Node descriptor */
typedef struct {
    int id; // SMPL facility identifier
    int *state; // State array. Might not be used?
    bool *pending; // Pending array for Ricart-Agrawala
    bool *waiting_reply;
    bool running;
    int clock; // Logical clock
    int timestamp; // Timestamp in which the node requested the critical region
} Node;
Node *nodes;

/* 
    Correct usage of program in case of incorrect parameters
*/
void show_usage() {
    printf("Correct usage: mutex [time...]\n");
    printf("time: moment in which each node will request usage of the critical region\n\n");
    printf("Example: \"mutex 10 20 30\" -> three nodes will request at timestamps 10, 20 and 30.\n");
}

void print_pending(bool p) {
    if (p) {
        printf(KGRN "%8s", "TRUE");
    } else {
        printf(KRED "%8s", "FALSE");
    }
    printf(RESET);
}

int max(int *a, int N) {
    int m = 0;
    for (int i=0; i<N; i++) {
        if (a[i] > m){
            m = a[i];
        }
    }
    return m;
}

bool empty(bool *a, int N) {
    for (int i=0; i<N; i++) {
        printf("%d ", a[i]);
        if (a[i]) {
            return false;
        }
    }
    printf("\n");
    return true;
}

bool priority(int id_a, int id_b, int timestamp_a, int timestamp_b) {
    if (timestamp_a < timestamp_b) {
        return true;
    }
    if (timestamp_a == timestamp_b && id_a < id_b) {
        return true;
    }
    return false;


}

/* Program body */
int main(int argc, char *argv[]) {
    static int N; // Number of nodes
    static int token;
    static int event;
    static int sender;
    static int timestamp;
    static int r;
    static int i;
    static char fa_name[5];

    if (argc < 2) {
        show_usage();
        exit(1);
    }

    N = argc-1;

    // Sets up SMPL simulation
    smpl(0, "mutual exclusion");
    reset();
    stream(1); // 1 execution thread

    // Node intialization
    nodes = (Node*) malloc(N*sizeof(Node));
    int times[N];
    for (i=0; i<N; i++) {
        memset (fa_name, '\0', 5);
        sprintf(fa_name, "%d", i);
        nodes[i].id = facility(fa_name, 1);
        nodes[i].running = false;
        nodes[i].clock = 0;
        nodes[i].pending = (bool *) malloc(N*sizeof(bool));
        memset (nodes[i].pending, false, N*sizeof(bool));

        nodes[i].waiting_reply = (bool *) malloc(N*sizeof(bool));
        memset (nodes[i].waiting_reply, false, N*sizeof(bool));

        times[i] = atoi(argv[i+1]);
        //nodes[i].time = atoi(argv[i+1]);
        nodes[i].timestamp = 1000;
    }

    // Schedules the C.R. requests
    for (i=0; i<N; i++) {
        schedule (REQUEST, times[i], i, i, 0);
    }

    //max(times, N)
    while (time() < 1000.0) {
        printf("%.1f\n", time());
        cause(&event, &token, &sender, &timestamp);
        Node node = nodes[token];
        printf("Node %d's logical clock is %d\n", token, nodes[token].clock);
        switch(event) {
            case REQUEST:
                // Schedule RECEIVE events for each of the other nodes in random times

                printf("%d is requesting C.R. with timestamp %d\n", token, nodes[token].clock);
                nodes[token].timestamp = nodes[token].clock;

                for (int i=0; i<N; i++) {
                    if (i==token) {
                        continue;
                    }
                    int delay = rand_int(0, MAX_MESSAGE_DELAY);
                    printf("Scheduling RECEIVE_REQUEST for %d at time %.1f and delay %d\n", i, time(), delay);
                    schedule (RECEIVE_REQUEST, delay, i, token, nodes[token].clock);
                    nodes[token].waiting_reply[i] = true;
                }

                //nodes[token].clock++;
                break;

            case RECEIVE_REQUEST:
                // Receive a REQUEST

                printf("%d is receiving request at timestamp %d from %d sent with timestamp %d\n", token, nodes[token].clock, sender, timestamp);

                nodes[token].clock = (nodes[token].clock > timestamp ? nodes[token].clock : timestamp)+1;

                if (priority(sender, token, timestamp, nodes[token].timestamp) && !nodes[token].running) {
                    int delay = rand_int(0, MAX_MESSAGE_DELAY);
                    printf("Scheduling RECEIVE_REPLY to %d at timestamp %d and delay %d\n", sender, nodes[token].clock, delay);
                    schedule(RECEIVE_REPLY, delay, sender, token, nodes[token].clock);
                } else {
                    printf("Adding %d to %d's pending array\n", sender, token);
                    nodes[token].pending[sender] = true;
                }

                break;
            case RECEIVE_REPLY:
                printf("%d is receiving reply at time %.1f from %d sent with timestamp %d\n", token, time(), sender, timestamp);
                
                nodes[token].waiting_reply[sender] = false;
                printf("%d\n", empty(nodes[token].waiting_reply, N));
                if (empty(nodes[token].waiting_reply, N)) {
                    printf("%d entered the critical region.\n", token);
                    schedule (RELEASE, RUNNING_INTERVAL, token, token, nodes[token].clock);
                    nodes[token].running = true;
                }

                nodes[token].clock = (nodes[token].clock > timestamp ? nodes[token].clock : timestamp)+1;
                break;
            case RELEASE:
                // Release the C.R. and alert other nodes
                printf("%d left the critical region.\n", token);
                nodes[token].clock++;
                nodes[token].running = false;
                for (int i=0; i<N; i++) {
                    if (nodes[token].pending[i]) {
                        int delay = rand_int(0, MAX_MESSAGE_DELAY);
                        printf("Scheduling RECEIVE_REPLY to %d at timestamp %d and delay %d\n", i, nodes[token].clock, delay);
                        schedule(RECEIVE_REPLY, delay, i, token, nodes[token].clock);
                        nodes[token].pending[i] = false;
                    }
                }
                break;
        }
        printf("Node %d's logical clock is %d\n", token, nodes[token].clock);
    }

}