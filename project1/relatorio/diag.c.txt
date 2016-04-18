/*
 *  Randomized distributed diagnosis algorithm
 *  Developed by Renan Greca
 *  https://github.com/RenanGreca/DistributedSystems  
 *
 *  Developed for Distributed Systems class
 *  Prof. Elias P. Duarte Jr.
 *  Universidade Federal do Paraná; Curitiba, Brazil
 *  April 2016
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
#define TEST 1
#define FAULT 2
#define REPAIR 3

/* Statuses */
#define UNKNOWN 0
#define FAULT_FREE 1
#define FAULTY 2
/*
SMPL defines FAULT_FREE as 0 and FAULTY as 1.
To simplify other parts of this code, it was
better to set UNKNOWN as 0. Therefore, when
using SMPL's status function, we must always
add 1.
*/

/* Time between each testing round */
#define TESTING_INTERVAL 30.0

/* Color constants for pretty printing */
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define RESET "\033[0m"

/* Node descriptor */
typedef struct {
    int id; // SMPL facility identifier
    int *state;
} Node;

Node *nodes;

/* 
    Correct usage of program in case of incorrect parameters
*/
void show_usage() {
    printf("Correct usage: diag [-s] [-v] [-a] [number-of-nodes]\n");
    printf("number-of-nodes: number of nodes for simulated system (must be an integer greater than 1)\n");
    printf("Options:\n");
    printf("        -s : Synthesis mode (simulates up to 30 events; prints and plots the statistics)\n");
    printf("        -v : Verbose (outputs a detailed log)\n");
    printf("        -a : Adaptive DSD (removes randomized node selection)\n");
}

/*
    Converts a status int into a readable string and prints it

    Input:
    int s: a status int (UNKNOWN, FAULT_FREE or FAULTY)

    FAULTY is in red
    FAULT-FREE is in green
    UNKNOWN is in yellow
*/
void print_status(int s) {
    if (s == FAULTY) {
        printf(KRED "%12s", "FAULTY");
    } else if (s == FAULT_FREE) {
        printf(KGRN "%12s", "FAULT-FREE");
    } else {
        printf(KYEL "%12s", "UNKNOWN");
    }
    printf(RESET);
}

/*
    Prints the actual state array, regardless of diagnosis status.
    For this to work, each node must know correctly its own state.
    Used only for simulation purposes, not feasible in an actual system.

    Input:
    Node *nodes: array of nodes
    int N: size of array
*/
void print_definitive_state(Node *nodes, int N) {
    int i;

    printf("%5s", "");
    for (i=0; i<N; i++) {
        printf("%12d", i);
    }

    printf("\n");
    printf("%5s", "");
    for (i=0; i<N; i++) {
        print_status(nodes[i].state[i]);
    }

    printf("\n");
    printf("------------------------------------------------------");
    printf("\n");
}

/*
    Prints all nodes' internal state arrays.

    Input:
    Node *nodes: array of nodes
    int N: size of array
*/
void print_states(Node *nodes, int N) {
    int i;
    // Prints the horizontal indexes
    printf("%5s", "");
    for (i=0; i<N; i++) {
        printf("%12d", i);
    }
    printf("\n");

    for (int k=0; k<N; k++) {
        if (nodes[k].state[k] == FAULT_FREE) {
            // Prints one vertical index
            printf("%4d:", k);
            for (i=0; i<N; i++) {
                print_status(nodes[k].state[i]);
            }
            printf("\n\n");
        }
    }
}

/*
    Checks if the system diagnosis is stable by comparing each node's
    internal state array to the actual state of the system (using the
    same method as print_definitive_states).

    Input:
    Node *nodes: array of nodes
    int N: size of array

    returns: true if system diagnosis is stable; false otherwise.
*/
bool stable(Node *nodes, int N) {
    for (int i=0; i<N; i++) {
        // if a node is faulty, its state array is disregarded
        if (nodes[i].state[i] == FAULT_FREE) {
            for (int j=0; j<N; j++) {
                if (nodes[i].state[j] != nodes[j].state[j]) {
                    return false;
                }
            }
        }
    }
    return true;
}

/*
    Computes the mean average and the standard deviation of the number
    of testing rounds each event took to be fully diagnosed.

    i.e. if a node failed during round 3 and the system became stable
    on round 7, the lag for that event was 4.

    The first lag is disregarded, because it's not representative of 
    the diagnosis algorithm. The first complete diagnosis will always
    take a long time because no node has any information on the others.

    Input:
    int *lag:   array of number of testing rounds between each event 
                and its diagnosis
    int n: number of events that occured during the simulation
    char *filename: Name of file in which to store output
*/
void mean_and_deviation(int *lag, int n, char *filename) {

    FILE *output = fopen(filename, "w");

    float mean = 0.0;
    for (int i=1; i<n; i++) {
        fprintf(output, "%d\n", lag[i]);
        mean += lag[i];
    }
    mean /= (n-1);

    fflush(output);

    float deviation = 0.0;
    for (int i=1; i<n; i++) {
        deviation += (lag[i] - mean)*(lag[i] - mean);
    }
    deviation /= (n-1);
    deviation = sqrt(deviation);

    printf("the mean was %.2f and the deviation was %.2f.\n", mean, deviation);
}

/* 
    Merges three state arrays together when a node acquires diagnosis
    information from another node.
    
    Input:
    int *temp: array with the results of the current round of tests.
    int *in: array with the states from the external node.
    int *out: the resulting array which will be stored in the node. 
*/
void merge(int *temp, int *in, int *out, int N) {
    for (int i=0; i<N; i++) {
        // Only valid data has to be copied; UNKNOWN states are useless.
        if ((temp[i] == FAULTY) || (temp[i] == FAULT_FREE)) {
            out[i] = temp[i];
        } else if ((in[i] == FAULTY) || (in[i] == FAULT_FREE)) {
            out[i] = in[i];
        }
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
    
    bool synthesis_mode = false;
    bool verbose_mode = false;
    bool adsd = false;

    // Parse command-line arguments
    int opt;
    if ((argc < 2) || (argc > 5)) {
        show_usage();
        exit(1);
    }

    while ((opt = getopt(argc, argv, "sva")) != -1) {
        switch (opt) {
            case 's': synthesis_mode = true; break;
            case 'v': verbose_mode = true; break;
            case 'a': adsd = true; break;
        default:
            show_usage();
            exit(1);
        }
    }

    N = atoi(argv[optind]);

    if (N <= 1) {
        show_usage();
        exit(1);
    }

    // Standard execution simulates 1 event
    // In synthesis mode, number of events is min(N/2, 30)
    int total_events = 1;
    if (synthesis_mode) {
        total_events = (N*2)/3;
        if (N > 35) {
            total_events = 30;
        }
    }

    printf("Number of events to be simulated: %d\n", total_events);

    // Sets up SMPL simulation
    smpl(0, "diagnosis");
    reset();
    stream(1); // 1 execution thread
    
    // Node intialization
    nodes = (Node*) malloc(N*sizeof(Node));
    for (i=0; i<N; i++) {
        memset (fa_name, '\0', 5);
        sprintf(fa_name, "%d", i);
        nodes[i].id = facility(fa_name, 1);
        nodes[i].state = (int *) malloc(N*sizeof(int));
        memset (nodes[i].state, UNKNOWN, N*sizeof(int));
        nodes[i].state[i] = FAULT_FREE;
    }
    
    // Schedules the initial testing round
    for (i=0; i<N; i++) {
        schedule (TEST, TESTING_INTERVAL, i);
    }

    // Prepare variables
    int *temp_state;
    temp_state = (int *) malloc(N*sizeof(int));

    int *lag;
    lag = (int *) malloc(total_events*sizeof(int));

    int *tests_lag;
    tests_lag = (int *) malloc(total_events*sizeof(int));

    int num_checked = 0;
    int testing_round = 0;
    int rounds_since_last_event = 0;
    int tests_since_last_event = 0;
    int number_of_tests = 0;
    int previous_token = 0;
    int number_of_events = -1;

    printf("=============================================================================\n");
    printf("                Randomized distributed diagnosis algorithm                   \n");
    printf("              Renan Greca - Distributed Systems, April 2016                  \n");
    printf("=============================================================================\n");

    if (verbose_mode) {
        printf("Current state arrays:\n");
        print_states(nodes, N);
    }

    // Repeats until the required number of events are simulated
    while (number_of_events < total_events) {
        cause(&event, &token);

        // if token < previous_token then a round of testing has just finished
        if (token < previous_token) {
            if (verbose_mode) {
                printf("Current state arrays:\n");
                print_definitive_state(nodes, N);
                print_states(nodes, N);
            }
         
            // If system is stable, generate an event
            if (stable(nodes, N) && rounds_since_last_event > 1) {

                // Stores lag information
                lag[number_of_events] = rounds_since_last_event;
                tests_lag[number_of_events] = tests_since_last_event;
                number_of_events++;

                if (verbose_mode) {
                    printf("System is stable after %d rounds, %d tests and %d events.\n",
                        rounds_since_last_event,
                        tests_since_last_event,
                        number_of_events);
                }

                // Selects a random node to fail
                // An already failed node may be chosen
                int event_node;
                do {
                    event_node = rand_int(0,N-1);
                } while (nodes[event_node].state[event_node] == FAULTY);

                if (verbose_mode) {
                    printf("Scheduling failure for %d in round %d\n", event_node, testing_round+1);
                }
                schedule (FAULT, 1.0, event_node);
                rounds_since_last_event = 0;
                tests_since_last_event = 0;
            }

            testing_round++;
            rounds_since_last_event++;
            if (verbose_mode) {
                printf("Testing for round %d\n", testing_round);
            }
        }

        switch(event) {
            case TEST:
                if(status(nodes[token].id)+1 != FAULT_FREE) {
                    break;
                }
                if (verbose_mode) {
                    printf("Node %d, about to test on round %d, time %5.1f\n", token, testing_round, time());
                }

                int j = token;
                int s = UNKNOWN;

                // temp_state stores the diagnosis information of the current round of testing.
                // this is useful both to avoid repeating random nodes and when merging state arrays.
                memset (temp_state, UNKNOWN, N*sizeof(int));
                temp_state[token] = FAULT_FREE;

                num_checked = 1;
                
                // Test random nodes until a FAULT_FREE one is found.
                do {
                    
                    // Non-random Adaptive DSD mode
                    if (adsd) {
                        j = (j+1) % N;
                    } else {
                        // Choose a random j until an unchecked one is found.
                        do {
                            // rand_int is derived from rand.c
                            j = rand_int(0,N-1);
                        } while ((temp_state[j] == FAULT_FREE || temp_state[j] == FAULTY) && num_checked < N);
                    }

                    // Now j is checked.
                    num_checked++;

                    // Tests j.
                    s = status(nodes[j].id)+1;
                    tests_since_last_event++;
                    number_of_tests++;

                    if (verbose_mode) {
                        printf("Testing node %d, whose status is: ", j);
                        print_status(s);
                        printf("\n");
                    }
                    
                    temp_state[j] = s;  

                } while (status(nodes[j].id)+1 != FAULT_FREE && num_checked < N);

                // Merge the state array.
                // If no fault-free node is found, only data found in the current round of testing will be used.
                merge(temp_state, nodes[j].state, nodes[token].state, N);

                schedule(TEST, TESTING_INTERVAL, token);
                break;
            case FAULT:
                r = request(nodes[token].id, token, 0);
                if (verbose_mode) {
                    if (status(nodes[token].id)+1 != FAULTY) {
                        printf("Unable to fail node %d\n", token);
                    }
                    printf("Node %d failed on round %d\n", token, testing_round);
                }
                nodes[token].state[token] = FAULTY;

                break;
            case REPAIR:
                release(nodes[token].id, token);
                if (verbose_mode) {
                    printf("Node %d recovered on round %d\n", token, testing_round);
                }
                nodes[token].state[token] = FAULT_FREE;

                schedule(TEST, TESTING_INTERVAL, token);
                break;
        }
        previous_token = token;
    }
    print_definitive_state(nodes, N);
    print_states(nodes, N);

    printf("Final state arrays after %d events, %d tests and %d rounds:\n", 
        number_of_events,
        number_of_tests,
        testing_round);

    if (synthesis_mode) {
        printf("For the number of rounds between each event, ");
        mean_and_deviation(lag, number_of_events, "rounds.out");
        printf("For the number of tests between each event, ");
        mean_and_deviation(tests_lag, number_of_events, "tests.out");
    }
}