The program includes 7 files all of them need to be executed in a single project.

POINTS TO BE NOTED:

1.The program prints the total number of connections made in graph and count of all the three lists for each day of the mentioned TIME_MAX.
2.There is also python code submitted for plotting the infection curves.
3.The time for code to be executed depends on number of max edges , more the number more the time.
IMPORTANT***
4.Since the entire process is being completed far less than the given time , recovery time is made to be min of 12 days :).
***
5.The algorithm is proven to halt after a finite time so your program is not really stuck.
6.Intrusive linked lists are used and SIR items for the list implementation are allocated contigously, also improving CPU cache efficiency.
7.The given Sample size is way too big it takes a considerable amount of time to run(approx 2 min), but keeping 1000 as sample size would do gud.



An individual represented as a node in a graph is initially Susceptible (S), then they might get
Infected (I) and then finally Recover (R).  We use a priority queue based on a dynamic
heap implementation.

1. Priority Queues

A priority queue is utilised to perform a discrete event simulation
for the purpose of simulating the epidemic. The events need to be
processed based on the timestamp (or virtual time) in the event struct
instead of the arrival time, hence a comparator is used to keep the
queue in sorted order for constant time extraction of highest priority
event. A dynamically resizing vector implementation is used as a
backend for the heap. The vector implementation uses a single
allocation but divides the pool into chunks which are a multiple of
the base unit of storage. This means that reallocations only occur for
every n insertions .

2. Graphs and Adjacency Matrices
A list based implementation is used instead of an adjacency matrix due to the big
size of input in the example test case (10000) which would grow the
memory usage rapidly.

An event struct is maintained in the priority queue Q, which will
necessarily have the following fields:

(*) The time when the event occurs
(*) The type of event (transmit or recover)
(*) The node corresponding to the event

Three global lists of S, I, and R nodes are also maintained for
bookkeeping purposes.


