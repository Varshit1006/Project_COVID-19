#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "graph.h"
#include "config.h"

#define POOL_SIZE (SAMPLE_SIZE * (NR_EDGES + 1))

extern List ListS;
extern List ListI;
extern List ListR;

bool sir_list_add_item(Node *n, List *l)                    //prepares struct sir and assigns node to it
{
	static size_t iterator = 0;
	static struct sir *pool = NULL;
	if (!l && !n) {
		free(pool);
		pool = NULL;
		iterator = 0;
		return true;
	}
	if (!pool) {
		pool = malloc(POOL_SIZE * sizeof *pool);
		if (!pool) return false;
	}
	struct sir *s = NULL;
	if (iterator < POOL_SIZE)
		s = pool + iterator++;
	if (!s) return false;
	s->list.next = NULL;
	list_append(l, &s->list);
	s->item = n;
	return true;
}

void sir_list_add_sir(struct sir *s, List *l)           //takes pre allocated struct sir
{
	assert(s);
	list_append(l, &s->list);
}

struct sir* sir_list_del_item(Node *n, List *l)         //to delete the node from list
{
	/* l is assumed to be anchor, not an object embedded in entry */
	struct sir *i;
	list_for_each_entry(i, l->next, struct sir, list) {
		if (i->item == n) {
			struct sir *f = container_of(&i->list, struct sir, list);
			/* updates l->next */
			list_delete(&l->next, &i->list);
			/* freeing individual elements is not possible
			   in pool based implementation */
			// free(f);
			f->list.next = NULL;
			return f;
		}
	}
	return NULL;
}

void sir_list_dump(List *l)             //helper function to print the nodes neighbors in graph
{
	struct sir *i;
	list_for_each_entry(i, l->next, struct sir, list)
		fprintf(stderr, "%u ", i->item->id);
	printf("");
}

void sir_list_del_rec(List *l)
{
	if (!l->next) return;
	struct sir *i;
	/* we cannot use list_for_each_entry here, since we need to
	 * advance the iterator and only then free the memory
	 */
	for (i = container_of(l->next, struct sir, list); i;) {
		struct sir *f = i;
		i = i->list.next ? container_of(i->list.next, struct sir, list) : NULL;
		free(f);
	}
	l->next = NULL;
}

size_t sir_list_len(List *l)            //returns the count of the given list
{
	/* begin counting from next, as l is anchor */
	size_t count = 0;
	while (l->next) {
		l = l->next;
		count++;
	}
	return count;
}

Node* node_new(size_t sz)               //function for creating a new node in graph
{
	Node *n = malloc(sz * sizeof(*n));
	if (!n) return NULL;
	for (size_t i = 0; i < sz; i++) {
		n[i].id = i + 1;
		n[i].state = SIR_SUSCEPTIBLE;
		n[i].neigh.next = NULL;
		n[i].tail = &n[i].neigh;
		n[i].initial = false;
	}
	return n;
}

void node_connect(Node *a, Node *b)     //connecting two nodes
{
	assert(a);
	assert(b);
	static bool conn_cache[SAMPLE_SIZE][SAMPLE_SIZE]={};

	struct sir *i;
	if (a == b) return;     //if they are the same node return without doing anything
	if(!conn_cache[a->id-1][b->id-1] || !conn_cache[b->id-1][a->id-1]){
        assert(!conn_cache[a->id-1][b->id-1] && !conn_cache[b->id-1][a->id-1]);
        conn_cache[a->id-1][b->id-1] = true;
        conn_cache[b->id-1][a->id-1] = true;

	}else return;
	sir_list_add_item(a,b->tail);
	b->tail = b->tail->next;
	sir_list_add_item(b,a->tail);
	a->tail = a->tail->next;
	max_conn++;
}

void node_dump_adjacent_nodes(Node *n)          //gives the neighbor nodes
{
	fprintf(stderr, "\nNode %u: ", n->id);
	sir_list_dump(&n->neigh);
}

void node_delete(Node *n)
{
	sir_list_del_rec(&n->neigh);
}
