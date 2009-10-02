#ifndef FLOWGRAPH_INCLUDED
#define FLOWGRAPH_INCLUDED

typedef struct Graph *FlowGraph;

FlowGraph Graph_new(int numVertices, int numEdges);
void Graph_free(FlowGraph g);
void Graph_addEdge(FlowGraph g, int from, int to, float capacity);
float Graph_maxflow(FlowGraph g);
float Graph_getFlow(FlowGraph g, int from, int to);
void Graph_resetFlows(FlowGraph g);

#endif /* FLOWGRAPH_INCLUDED */
