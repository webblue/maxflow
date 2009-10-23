import maxflowhelper


class GraphError(Exception):
    def __init__(self, msg=None):
        Exception.__init__(self, msg)


class FlowGraph(object):
    SOURCE_ID = 0
    SINK_ID = 1

    def __init__(self):
        '''
        Creates an empty flow graph.
        '''
        self.edgesbyid = {}      # Maps (tailid, headid) to [tailid, headid, capacity, flow] list
        self.edgesbyname = {}    # Maps (tailname, headname) to same edge as above
        self.vertexname2id = {}  # Maps vertex names to their ids
        self.v2vertices = {}     # Maps vertex names to vertices it points to
        self.vertices2v = {}     # Maps vertex names to vertices that point to it
        self.nextvertexid = 2    # 0 is source "s", 1 is sink "t"

    def numvertices(self):
        return self.nextvertexid

    def numedges(self):
        return len(self.edgesbyid)

    def _getvertexid(self, vertex):
        if vertex == 's':
            self.vertexname2id['s'] = FlowGraph.SOURCE_ID
            return FlowGraph.SOURCE_ID
        if vertex == 't':
            self.vertexname2id['t'] = FlowGraph.SINK_ID
            return FlowGraph.SINK_ID
        if vertex in self.vertexname2id:
            return self.vertexname2id[vertex]
        newid = self.nextvertexid
        self.nextvertexid += 1
        self.vertexname2id[vertex] = newid
        return newid

    def alterflow(self, tail, head, flow, additive=False):
        '''
        Sets the flow from tail to head to flow.  If additive is True,
        it adds flow to the existing flow instead of replacing it.
        '''
        try:
            if additive:
                self.edgesbyname[(tail, head)][3] += flow
            else:
                self.edgesbyname[(tail, head)][3] = flow
        except KeyError:
            raise GraphError('there is no edge from %s to %s' % (tail, head))

    def addedge(self, tail, head, cap, increaseifexists=False):
        '''
        Adds an edge from vertex tail to vertex head with
        floating-point capacity cap.  Vertices can be named using any
        hashable Python object.  If increaseifexists is True and the
        edge already exists, its capacity is increased by the given
        capacity instead of replaced.
        '''
        if (tail, head) in self.edgesbyname:
            if increaseifexists:
                self.edgesbyname[(tail, head)][2] += cap
            else:
                self.edgesbyname[(tail, head)][2] = cap
        else:
            tailid = self._getvertexid(tail)
            headid = self._getvertexid(head)
            edge = [tailid, headid, cap, 0.0]
            self.edgesbyid[(tailid, headid)] = edge
            self.edgesbyname[(tail, head)] = edge
            if tail in self.v2vertices:
                self.v2vertices[tail].append(head)
            else:
                self.v2vertices[tail] = [head]
            if head in self.vertices2v:
                self.vertices2v[head].append(tail)
            else:
                self.vertices2v[head] = [tail]

    def calculatemaxflow(self):
        '''
        Calculates the max flow from vertex "s" to vertex "t" and
        returns the resulting scalar.  After running this method, call
        calculatemaxflow() to retrieve the flows on individual edges.
        The bulk of this method is implemented in C for efficiency.
        '''
        if 's' not in self.vertexname2id or 't' not in self.vertexname2id:
            raise GraphError('graph must have a source named "s" and a sink named "t"')
        # Call C helper for speed.
        return maxflowhelper.maxflow(self.edgesbyid.values(), self.nextvertexid)

    def getflow(self, tail, head):
        '''
        Returns the flow from vertex tail to vertex head.  The
        returned flow is 0.0 if there is no edge from tail to head.
        '''
        if tail not in self.vertexname2id:
            raise GraphError('unknown vertex "%s"' % tail)
        if head not in self.vertexname2id:
            raise GraphError('unknown vertex "%s"' % head)
        try:
            return self.edgesbyname[(tail, head)][3]
        except KeyError:
            return 0.0

    def getcapacity(self, tail, head):
        '''
        Returns the capacity from vertex tail to vertex head.  The
        returned capacity is 0.0 if there is no edge from tail to
        head.
        '''
        if tail not in self.vertexname2id:
            raise GraphError('unknown vertex "%s"' % tail)
        if head not in self.vertexname2id:
            raise GraphError('unknown vertex "%s"' % head)
        try:
            return self.edgesbyname[(tail, head)][2]
        except KeyError:
            return 0.0

    def getverticesfrom(self, tail):
        try:
            return self.v2vertices[tail]
        except KeyError:
            return []

    def getverticesto(self, head):
        try:
            return self.vertices2v[head]
        except KeyError:
            return []
