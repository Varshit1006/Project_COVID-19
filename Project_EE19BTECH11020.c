#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


#include "config.h"
#include "prioq.h"
#include "graph.h"


/* TODO:
 * Ensure that the SAMPLE_SIZE is no bigger than RAND_MAX
 * Find some comprehensive fix to UAFs from vector_reset
 */



// anchors for SIR lists
List ListS;
List ListI;
List ListR;
size_t max_conn = 0;

#define DUMP_NUM  0x00000001           //these are the flags to select what to print in dump_stats function
#define DUMP_NODE 0x00000002
#define DUMP_SIR  0x00000004

int data[TIME_MAX][3];              //2d array to maintain record of SIR list for each day

static void dump_stats(Node *n, unsigned mask)              //prints the relevant information asked
{
	if (mask & DUMP_NUM) {
		printf("Sample Size:        %u \n", SAMPLE_SIZE);
		printf("Max edges:          %u \n", NR_EDGES);
		printf("Connections made:   %zu \n", max_conn);
		printf("Infected people in the end:    %zu \n", sir_list_len(&ListI));
	}
	if (mask & DUMP_SIR) {
		printf("Susceptible: "); sir_list_dump(&ListS);
		printf("\nInfected: "); sir_list_dump(&ListI);
		printf("\nRecovered: "); sir_list_dump(&ListR);
	}
	if (mask & DUMP_NODE) {
		assert(n);
		for (size_t i = 0; i < SAMPLE_SIZE; i++)
			node_dump_adjacent_nodes(n + i);
	}
	printf("\n================================\n");
}

int main(void)
{
	PriorityQueue *pq = NULL;           //initialise a new priority queue
	Node *narr = NULL;                  //initialise nodes
	int r;

	if (NR_EDGES > SAMPLE_SIZE - 1) {
		printf("Incorrect NR_EDGES value configured.\n");
		return 1;
	}

	pq = pq_new();
	if (!pq) {
		printf("Failed to setup priority queue, fatal.\n");
	}

	narr = node_new(SAMPLE_SIZE);
	if (!narr) {
		printf("Failed to allocate nodes, fatal.\n");
	}

	for (size_t i = 0; i < SAMPLE_SIZE; i++) {
		if (!sir_list_add_item(narr + i, &ListS)) {
			printf("Failed to add nodes to Susceptible list, fatal.\n");
		}
	}

	//printf("Initial lists: \n");
	//dump_stats(NULL, DUMP_SIR);       //prints the initial lists of SIR at start


	// now connect nodes, randomly
	for (size_t i = 0; i < SAMPLE_SIZE; i += 2) {
		int c = 0;
		c = gen_random_id(NR_EDGES+1, -1);
		while (c--) {
			size_t j = gen_random_id(SAMPLE_SIZE, i);
			node_connect(&narr[i], &narr[j]);
		}
	}
	//printf("Node connections: \n");
	//dump_stats(narr, DUMP_NODE);          //prints the graph i.e prints each node and its respective connections

	size_t infect = gen_random_id(SAMPLE_SIZE/20, 0);      //for initial spreaders
	printf("INITIAL SPREADERS = %d\n",infect);

	while (infect--) {
		size_t r = gen_random_id(SAMPLE_SIZE, -1);
		if (narr[r].initial == true) {
			continue;
		}
		PQEvent *ev = pqevent_new(narr + r, TRANSMIT);
		if (!ev) {
			continue;
		}
		ev->timestamp = 0;
		if (!pqevent_add(pq, ev)) {
			pqevent_delete(ev);
			continue;
		}
		//printf("\nAdded TRANSMIT event for initial spreader %u with time %lu \n", narr[r].id, ev->timestamp);

		ev = pqevent_new(narr + r, RECOVER);
		if (!ev) {
			printf("\nFailed to allocate RECOVER event for spreader %u.\n", narr[r].id);
			continue;
		}
		ev->timestamp = toss_coin(0, ev->Y) + 12;
		if (!pqevent_add(pq, ev)) {
            printf("\nFailed to add RECOVER event for spreader %u.\n", narr[r].id);
			pqevent_delete(ev);
			continue;
		}
		//printf("\nAdded RECOVER event for initial spreader %u with time %lu \n", narr[r].id, ev->timestamp);

		narr[r].initial = true;
	}

	// begin simulation
	PQEvent *ev;
	for (ev = pqevent_next(pq); ev && ev->timestamp < TIME_MAX; ev = pqevent_next(pq)) {
	    int day = 0;

		if (ev->type == TRANSMIT) {
			if (UINT_IN_SET(ev->node->state, SIR_SUSCEPTIBLE) ||
				(ev->timestamp == 0 && ev->node->state == SIR_INFECTED)) {
				//printf("\nProcessing event TRANSMIT at time %lu for Node %u \n", ev->timestamp, ev->node->id);
				process_trans_SIR(pq, ev);
			}
		} else if (ev->type == RECOVER && ev->node->state != SIR_RECOVERED) {
			//printf("\nProcessing event RECOVER at time %lu for Node %u \n", ev->timestamp, ev->node->id);
			process_rec_SIR(pq, ev);
		} /* else skip the event */

		data[ev->timestamp][0] = sir_list_len(&ListS);
		data[ev->timestamp][1] = sir_list_len(&ListI);
		data[ev->timestamp][2] = sir_list_len(&ListR);

		pqevent_delete(ev);


	}
	if (ev) {
		/* min-heap was not empty */
		/* caught by LSAN: this is a min heap, so a zero
		initialized PQEvent is not really a good end marker,
		since it would swap its way up to the top, and cut
		short our loop below, leaking memory, hence make the
		timestamp infinite */
		pqevent_add(pq, &(PQEvent){ .timestamp = -1 });
		do {
			pqevent_delete(ev);
		} while (ev = pqevent_next(pq), ev && ev->node);
	}
	dump_stats(narr,DUMP_NUM);
	//printf("\nFinal lists: \n");
	//dump_stats(narr,DUMP_SIR);            //prints the final lists of SIR by end of time max.

    data[0][0] = SAMPLE_SIZE;           //initializing everyone to susceptible list
	for(int t=0 ; t<= TIME_MAX;t++){
        if(data[t][0] == 0 && data[t][1] ==0 && data[t][2] == 0){       //initial spreaders are not counted from day 1 because they are added manually
            data[t][0] = data[t-1][0];
            data[t][1] = data[t-1][1];
            data[t][2] = data[t-1][2];
        }
	}
	    printf("Daily check:\n");
	    FILE*f;
	    f = fopen("input.dat","w");
	for(int t=0 ; t< TIME_MAX;t++){
        printf("DAY %d : Susceptible-%zu , Infected-%zu , Recovered-%zu.\n",t+1,data[t][0],data[t][1],data[t][2]);      //prints the daily count of S,I,R
        fprintf(f,"%d %zu %zu %zu\n",t+1,data[t][0],data[t][1],data[t][2]);
	}
	fclose(f);
	printf("Destructing objects...\n");
	// resets pool
	sir_list_add_item(NULL, NULL);
	free(narr);
	pq_delete(pq);
	return r;
}
