# sequoia

The aim of this library is to provide highly-customizable mathematical concepts.
It is very much a work in progress and is still very rough around the edges.
However, certain things are working.

In particular, it is possible to create constexpr graphs.

For example

(i) Undirected graphs

using g_type = static_graph<flavour::undirected, 2, 1, null_weight, double>;
using edge = g_type::edge_init_type;

constexpr g_type g{{edge(1,1.4)}, {edge(0,1.4)}};

This creates a directed graph with 2 nodes joined by one edge, with the nodes carrying no weight and the edge carrying a double, like this:

X--1.4--X

In this case, the first argument of each edge carries the node index to which the edge goes, whereas the second carries the weight. If a graph which represents pure connectivity is required, then the edges may be designated to carry null_weight (or indeed any empty class) and no memory is wasted on this.

Furthermore, if the edge indices are all small enough to be stored as chars, then this can be exploited by using the final template agument, which defaults to std::size_t, viz.:

static_graph<flavour::undirected, 2, 1, null_weight, double, char>;

(ii) Undirected Embedded graphs

Graphs of this type are undirected but exploit a cyclic ordering of the edges on each node.

using g_type = static_graph<flavour::undirected_embedded, 2, 3, null_weight, double>;
using edge = g_type::edge_init_type;
constexpr g_type g{{edge{1,0,1.4}, edge{1,2,2.0}, edge{1,1,0.7}}, {edge{0,0,1.4}, edge{0, 1, 2.0}, edge{0, 2, 0.7}}};

In this case, the second argument of each edge denotes the position of this edge on the target node.

(iii) Directed graphs

using g_type = static_graph<flavour::directed, 2, 2, null_weight, double, true, char>;
using edge = g_type::edge_init_type;

constexpr g_type g{{edge{1,1.4}}, {edge{0,0.6}}};

This creates a directed graph with two nodes and two edges.

For this flavour of graph, each node contains the list of edges which leave the node, with the first index of the node denoting the incident node. 

(iv) Directed embedded graphs

In this case an edge is recorded for both the node it leaves and the node to which it attaches.

using g_type = static_graph<flavour::directed_embedded, 2, 1, null_weight, double>;
using edge = g_type::edge_init_type;

constexpr g_type g{{edge{0,1,0,1.4}}, {edge{0,1,0,1.4}}};

Though in a raw state, this library is heavily tested: there are currently >64k checks performed in the unit/performance tests.

In addition to providing static graphs, dynamic graphs are supported within the same framework. Beyond being able to specify the edge/node weights and whether a graph is (un)directed, clients may also choose a policy for the manner in which the weights are stored, for example via a data pool.

Several traversal methods are offered; as nodes/edges are encountered functors may be exectued. The traversal methods accept a policy which determines whether these functors are executed in serial, asynchronously, via a thread pool...

As befits any C++ library, you don't pay for what you don't use. So if, for example, you set up a graph with null weights for both the edges and nodes, zero space is taken up by the nodes, while the edges just consist of the relevent index/indices.
