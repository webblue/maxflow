This package implements the Edmonds-Karp algorithm for finding the
maximum flow in a flow graph.  Most of it is written in C using the
Python C API, with a thin wrapper around the graph structure for
creating the graph and retrieving the flow values after the max flow
has been computed.

Like usual, install this package with

     python setup.py build
     python setup.py install

Example usage:

    from maxflow import FlowGraph
    g = FlowGraph()
    g.addedge('s', 'top', 5.0)     # s is the source vertex
    g.addedge('s', 'bottom', 4.0)
    g.addedge('top', 't', 3.0)     # t is the sink vertex
    g.addedge('bottom', 't', 9.0)
    maxflowval = g.calculatemaxflow()
    print 'max flow is', maxflowval
    print 'max flow from "s" to "top" is', g.getflow('s', 'top')

Vertices can be named using any hashable Python object.

Known issues:
    - Behavior is undefined if the graph has any self edges or
      two-vertex cycles (an edge (x, y) and (y, x)).
