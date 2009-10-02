#include <stdlib.h>
#include <string.h>
#include "flowgraph.h"
#include "tablefixed.h"

#define DEGREE_ALLOC 10
#define INFINITY     1000000.0
#define SOURCE_ID 0
#define SINK_ID   1

/* This MIN is not safe if X or Y have side effects, so be careful! */
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))


struct Edge {
    float capacity;
    float flow;
    struct Vertex *from;
    struct Vertex *to;
    /* This is used only for computing maxflow; it's not part of the actual graph. */
    struct Edge *reverseEdge;
};

struct Vertex {
    int id;
    int degree;
    int slots;
    struct Edge **edges;  /* Array of pointers to Edges */
    /* This is used only for computing maxflow; it's not part of the actual graph. */
    struct Edge *predEdge;
};

struct Graph {
    TableFixed_T edges;     /* Maps (to, from) to Edge  */
    TableFixed_T vertices;  /* Maps vertex id to Vertex */
    struct Vertex *source;
    struct Vertex *sink;
    int numVertices;
    int numEdges;
};


FlowGraph Graph_new(int numVertices, int numEdges)
{
    FlowGraph g = (FlowGraph) malloc(sizeof(*g));
    g->vertices = TableFixed_new(numVertices, sizeof(int));
    g->edges = TableFixed_new(numEdges, 2 * sizeof(int));
    g->numVertices = 0;
    g->numEdges = 0;
    return g;
}

void Graph_free(FlowGraph g)
{
    struct Vertex *v;
    TableFixedIter_T iter = TableFixedIter_new(g->vertices);
    TableFixedIter_selectFirst(iter);
    while(TableFixedIter_valid(iter)) {
        v = (struct Vertex *) TableFixedIter_selectedValue(iter);
        free(v->edges);
        free(v);
        free(TableFixedIter_selectedKey(iter));
        TableFixedIter_selectNext(iter);
    }
    TableFixedIter_free(iter);
    TableFixed_free(g->vertices);

    iter = TableFixedIter_new(g->edges);
    TableFixedIter_selectFirst(iter);
    while(TableFixedIter_valid(iter)) {
        free(TableFixedIter_selectedValue(iter));
        free(TableFixedIter_selectedKey(iter));
        TableFixedIter_selectNext(iter);
    }
    TableFixedIter_free(iter);
    TableFixed_free(g->edges);
    free(g);
}

static struct Vertex *addVertex(FlowGraph g, int id)
{
    struct Vertex *v;
    int *key;
    v = (struct Vertex *) TableFixed_getValue(g->vertices, &id);
    if(!v) {
        key = (int *) malloc(sizeof(*key));
        *key = id;
        v = (struct Vertex *) malloc(sizeof(*v));
        v->id = id;
        v->degree = 0;
        v->slots = DEGREE_ALLOC;
        v->edges = (struct Edge **) malloc(v->slots * sizeof(struct Edge *));
        v->predEdge = NULL;
        TableFixed_put(g->vertices, key, v);
        if(id == SOURCE_ID)
            g->source = v;
        else if(id == SINK_ID)
            g->sink = v;
        g->numVertices++;
    }
    return v;
}

static void connectVtoE(struct Vertex *v, struct Edge *e)
{
    if(v->slots <= v->degree) {
        /* Double the size of the allocated array for future additions. */
        v->slots *= 2;
        v->edges = (struct Edge **) realloc(v->edges, v->slots * sizeof(struct Edge *));
    }
    v->edges[v->degree++] = e;
}

static const void *createKey(int from, int to)
{
    void *key = malloc(2 * sizeof(int));
    memcpy(key, &from, sizeof(int));
    memcpy(key + sizeof(int), &to, sizeof(int));
    return key;
}

void Graph_addEdge(FlowGraph g, int from, int to, float capacity)
{
    struct Vertex *vfrom, *vto;
    const void *key = createKey(from, to);
    struct Edge *e = (struct Edge *) malloc(sizeof(*e));
    vfrom = addVertex(g, from);
    vto = addVertex(g, to);

    e->capacity = capacity;
    e->flow = 0.0;
    e->from = vfrom;
    e->to = vto;
    e->reverseEdge = NULL;
    TableFixed_put(g->edges, key, e);

    g->numEdges++;
    connectVtoE(vfrom, e);
}

float Graph_getFlow(FlowGraph g, int from, int to)
{
    int key[] = {from, to};
    return ((struct Edge *) TableFixed_getValue(g->edges, key))->flow;
}

/* --------------------------------------------------------------------------- */
/* maxflow algorithm here is an adaptation of the Ford-Fulkerson
   algorithm as presented in
   http://www.aduni.org/courses/algorithms/courseware/handouts/Reciation_09.html. */

struct MaxFlowInfo {
    /* Keeps a handle on edges created only for maxflow computation so
       we can free them when done. */
    struct Edge **reverseEdges;
    int numReverseEdges;
};

static void enqueue(struct MaxFlowInfo *mfi, struct Vertex *v)
{
    mfi->queue[mfi->tail++] = v;
    mfi->visited[v->id] = 1;
}

static struct Vertex *dequeue(struct MaxFlowInfo *mfi)
{
    struct Vertex *v = mfi->queue[mfi->head++];
    mfi->visited[v->id] = BLACK;
    return v;
}

static float maxflowIteration(FlowGraph g, struct MaxFlowInfo *mfi, struct Vertex *start)
{
    struct Edge *e;
    int i;
    float nextIncrement;

    if(start == g->sink)
        return INFINITY;

    while(i = 0; i < start->degree; i++) {
        e = start->edges[i];
        if(e->capacity - e->flow > 0.0) {
            nextIncrement = maxflowIteration(g, mfi, e->to);
            if(nextIncrement > 0.0)
                return MIN(increment, nextIncrement);
        }
    }
}

static void addFlow(struct MaxFlowInfo *mfi, struct Edge *edge, float amount)
{
    struct Edge *reverse = edge->reverseEdge;
    if(!reverse) {
        /* Create a temporary reverse edge and add it to the list of
           reverse edges for removal after the maxflow algorithm
           completes. */
        reverse = (struct Edge *) malloc(sizeof(*reverse));
        reverse->capacity = 0.0;
        reverse->flow = 0.0;
        reverse->from = edge->to;
        reverse->to = edge->from;
        reverse->reverseEdge = edge;
        edge->reverseEdge = reverse;
        mfi->reverseEdges[mfi->numReverseEdges++] = reverse;
        connectVtoE(reverse->from, reverse);
    }
    edge->flow += amount;
    reverse->flow -= amount;
}

float Graph_maxflow(FlowGraph g)
{
    int n = g->numVertices, i;
    struct MaxFlowInfo mfi;
    struct Vertex *u, *v;
    struct Edge *e;
    float increment, maxflowVal = 0.0;
    mfi.reverseEdges = (struct Edge **) malloc(g->numEdges * sizeof(struct Edge *));
    mfi.numReverseEdges = 0;
    
    do {
        increment = maxflowIteration(g, &mfi, g->source);
        maxflowVal += increment;
    } while(increment > 0.0);

    for(i = 0; i < mfi.numReverseEdges; i++) {
        e = mfi.reverseEdges[i];
        e->reverseEdge->reverseEdge = NULL;
        e->from->degree--;
        free(e);
    }
    free(mfi.reverseEdges);
    return maxflowVal;
}

void Graph_resetFlows(FlowGraph g)
{
    struct Edge *e;
    TableFixedIter_T iter = TableFixedIter_new(g->edges);
    TableFixedIter_selectFirst(iter);
    while(TableFixedIter_valid(iter)) {
        e = (struct Edge *) TableFixedIter_selectedValue(iter);
        e->flow = 0.0;
        TableFixedIter_selectNext(iter);
    }
    TableFixedIter_free(iter);
}
