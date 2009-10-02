from distutils.core import setup, Extension

setup(name='MaxFlow',
      version='0.1',
      author='Ryan S. Peterson',
      author_email='ryanp@cs.cornell.edu',
      url='http://www.cs.cornell.edu/~ryanp/',
      description='A Python-wrapped C implementation of the Ford-Fulkerson algorithm for finding the maximum flow in a flow graph.',
      license='MIT',
      packages=['maxflow'],
      package_dir={'maxflow': 'src'},
      ext_modules=[Extension('maxflow.maxflowhelper', ['src/maxflowhelper/maxflowhelper.c', 'src/maxflowhelper/flowgraph.c', 'src/maxflowhelper/tablefixed.c'])])
