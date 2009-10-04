#include "Python.h"
#include "flowgraph.h"


static FlowGraph constructGraph(PyObject *edges, int numVertices)
{
    PyObject *item, *iter;
    int numEdges = PyList_Size(edges), from, to;
    FlowGraph g = Graph_new(numVertices, numEdges);
    float capacity;

    iter = PyObject_GetIter(edges);
    while((item = PyIter_Next(iter))) {
        from = PyInt_AsLong(PyList_GetItem(item, 0));
        to = PyInt_AsLong(PyList_GetItem(item, 1));
        capacity = (float) PyFloat_AsDouble(PyList_GetItem(item, 2));
        Graph_addEdge(g, from, to, capacity);
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    return g;
}

static void copyFlowsToPython(FlowGraph graph, PyObject *edges)
{
    PyObject *item, *iter;
    int from, to;
    float flow;

    iter = PyObject_GetIter(edges);
    while((item = PyIter_Next(iter))) {
        from = PyInt_AsLong(PyList_GetItem(item, 0));
        to = PyInt_AsLong(PyList_GetItem(item, 1));
        flow = Graph_getFlow(graph, from, to);
        PyList_SetItem(item, 3, PyFloat_FromDouble(flow));
        Py_DECREF(item);
    }
    Py_DECREF(iter);
}

static PyObject *maxflow(PyObject *self, PyObject *args)
{
    PyObject *edges;
    int numVertices;
    FlowGraph graph;
    float maxflowVal;

    if(!PyArg_ParseTuple(args, "Oi", &edges, &numVertices))
        return NULL;
    if(!PyList_Check(edges))
        return NULL;
    graph = constructGraph(edges, numVertices);
    Py_BEGIN_ALLOW_THREADS
    maxflowVal = Graph_maxflow(graph);
    Py_END_ALLOW_THREADS
    copyFlowsToPython(graph, edges);
    Graph_free(graph);
    return Py_BuildValue("f", maxflowVal);
}

static PyMethodDef maxflowMethods[] = {
    {"maxflow",  maxflow, METH_VARARGS,
     "Finds the max flow of the input graph."},
    {NULL, NULL, 0, NULL}  /* Sentinel (terminates structure) */
};

PyMODINIT_FUNC initmaxflowhelper(void)
{
    Py_InitModule("maxflowhelper", maxflowMethods);
}
