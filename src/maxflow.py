import maxflowhelper


class FlowGraph(object):
    def __init__(self):
        '''
        Creates an empty flow graph.
        '''
        self.edges = {}        # Maps (tailid, headid) to [tailid, headid, capacity, flow] list
        self.vname2id = {'s': 0, 't': 1}  # Maps vertex name to vertex id
        self.vid2name = {0: 's', 1: 't'}  # Maps vertex id to vertex name
        self.nextvertexid = 2  # 0 is source "s", 1 is sink "t"

    def _getvertexid(self, vertex):
        if vertex == 's':
            return 0
        if vertex == 't':
            return 1
        if vertex in self.vname2id:
            return self.vname2id[vertex]
        id = self.nextvertexid
        self.nextvertexid += 1
        self.vname2id[vertex] = id
        self.vid2name[id] = vertex
        return id

    def addedge(self, tail, head, cap):
        '''
        Adds an edge from vertex tail to vertex head with
        floating-point capacity cap.  Vertices can be named using any
        hashable Python object.
        '''
        tailid = self._getvertexid(tail)
        headid = self._getvertexid(head)
        edge = [tailid, headid, cap, 0.0]
        self.edges[(tailid, headid)] = edge

    def calculatemaxflow(self):
        '''
        Calculates the max flow from vertex "s" to vertex "t" and
        returns the resulting scalar.  After running this method, call
        getflow() to retrieve the flows on individual edges.  The bulk
        of this method is implemented in C for efficiency.
        '''
        assert 's' in self.vname2id and 't' in self.vname2id, \
               'graph must have a source named "s" and a sink named "t"'
        # Call C helper for speed.
        return maxflowhelper.maxflow(self.edges.values(), self.nextvertexid)

    def getflow(self, tail, head):
        '''
        Returns the flow from vertex tail to vertex head.  The
        returned flow is 0.0 if there is no edge from tail to head.
        '''
        assert tail in self.vname2id, 'unknown vertex "%s"' % tail
        assert head in self.vname2id, 'unknown vertex "%s"' % head
        try:
            return self.edges[(self.vname2id[tail], self.vname2id[head])][3]
        except KeyError:
            return 0.0
