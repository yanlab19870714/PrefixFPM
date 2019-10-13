//########################################################################
//## Copyright 2019 Da Yan http://www.cs.uab.edu/yanda
//##
//## Licensed under the Apache License, Version 2.0 (the "License");
//## you may not use this file except in compliance with the License.
//## You may obtain a copy of the License at
//##
//## //http://www.apache.org/licenses/LICENSE-2.0
//##
//## Unless required by applicable law or agreed to in writing, software
//## distributed under the License is distributed on an "AS IS" BASIS,
//## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//## See the License for the specific language governing permissions and
//## limitations under the License.
//########################################################################

//########################################################################
//## Contributors
//## * Wenwen Qu
//## * Da Yan
//########################################################################

#ifndef GSPANTRANS_H_
#define GSPANTRANS_H_

#include <iterator>
#include <map>
#include "assert.h"
#include "Utils.h"

using namespace std;

//used in transaction and projected transaction, point to data graph
struct Edge {
	int from;
	int to;
	int elabel;
	unsigned int id; // used in History to track whether an edge is visited
};


class Vertex
{
public:
	typedef vector<Edge>::iterator edge_iterator;

	int label;
	vector<Edge> edge;

	void push (int from, int to, int elabel)
	{
		edge.resize (edge.size()+1);
		Edge& e = edge.back();
		e.from = from;
		e.to = to;
		e.elabel = elabel;
	}
};

class GspanTrans: public Trans{
public:
	vector<Vertex> graph; //data graph
	typedef vector<Vertex>::iterator vertex_iterator;
	unsigned int edge_size_ = 0;

	GspanTrans(){};
	GspanTrans(int _tid): edge_size_(0), Trans(_tid){};

	unsigned int edge_size ()   { return edge_size_; }
	unsigned int vertex_size () { return (unsigned int)graph.size(); } // wrapper

	void buildEdge () // attach edge id to each edge in the graph
	{
		char buf[512];
		map <string, unsigned int> tmp; //tmp[<from, to, elabel>] = edge_id

		unsigned int id = 0;
		for (int from = 0; from < (int)graph.size (); ++from) {
			for (Vertex::edge_iterator it = this->graph[from].edge.begin ();
					it != this->graph[from].edge.end (); ++it)
			{
				if (directed || from <= it->to)
					sprintf (buf, "%d %d %d", from, it->to, it->elabel);
				else
					sprintf (buf, "%d %d %d", it->to, from, it->elabel);

				// Assign unique id's for the edges.
				if (tmp.find (buf) == tmp.end()) {
					it->id = id;
					tmp[buf] = id;
					++id;
				} else {
					it->id = tmp[buf];
				}
			}
		}

		edge_size_ = id;
	}

	//read graph from istream, once a graph
	istream &read (istream &is)
	{
		vector <string> result;
		char line[1024];

		graph.clear ();
		/*
		 * "t" in the istream marks the end of one graph and will break the loop.
		 * "v" marks the vertex of graph. And if it doesn't appear in g(Graph class), then insert into g.
		 * "e" marks the edge of graph. If it doesn't appear in g, insert it into from-vertex's adject list.
		 * */
		while (true) {
			unsigned int pos = is.tellg ();
			if (! is.getline (line, 1024))
				break;
			result.clear ();
			tokenize<string>(line, back_inserter (result));
			if (result.empty()) {
				// do nothing
			} else if (result[0] == "t") {
				if (! graph.empty()) { // use as delimiter
					//is.seekg (pos, ios_base::beg); //removed from orginial code as this is a big slowdown in graph loading...
					break;
				}
			} else if (result[0] == "v" && result.size() >= 3) {
				unsigned int id    = atoi (result[1].c_str());
				this->graph.resize (id + 1);
				this->graph[id].label = atoi (result[2].c_str());
			} else if (result[0] == "e" && result.size() >= 4) {
				int from   = atoi (result[1].c_str());
				int to     = atoi (result[2].c_str());
				int elabel = atoi (result[3].c_str());

				if ((int)graph.size () <= from || (int)graph.size () <= to) {
					//cerr << "Format Error:  define vertex lists before edges" << endl;
					//exit (-1);
					//just for experiment, skip broken line
					continue;
				}

				this->graph[from].push (from, to, elabel);
				if (directed == false)
					this->graph[to].push (to, from, elabel);
			}
		}
		return is;
	}
	//write graph to ostream
	void write (ostream& ostr)
	{
		char buf[512];
		set <string> tmp;

		for (int from = 0; from < (int)graph.size (); ++from) {
			ostr << "v " << from << " " << this->graph[from].label << endl;

			for (Vertex::edge_iterator it = this->graph[from].edge.begin ();
					it != this->graph[from].edge.end (); ++it) {
				if (directed || from <= it->to) {
					sprintf (buf, "%d %d %d", from, it->to,   it->elabel);
				} else {
					sprintf (buf, "%d %d %d", it->to,   from, it->elabel);
				}
				tmp.insert (buf);
			}
		}

		for (set<string>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
			ostr << "e " << *it << ::endl;
			}
	}
};

//the elements in DFSCodes, which used to code a graph. Each DFS represents one edge.
class DFS{
public:
	int from;
	int to;
	int fromlabel;
	int elabel;
	int tolabel;
	friend bool operator == (const DFS &d1, const DFS &d2)
	{
		return (d1.from == d2.from && d1.to == d2.to && d1.fromlabel == d2.fromlabel
			&& d1.elabel == d2.elabel && d1.tolabel == d2.tolabel);
	}
	friend bool operator != (const DFS &d1, const DFS &d2) { return (! (d1 == d2)); } //used to check whether two DFS is equal in is_min()

	DFS(){};
	DFS(int _from, int _to, int _flabel, int _elabel, int _tlabel): from(_from), to(_to), fromlabel(_flabel), elabel(_elabel), tolabel(_tlabel) {};
};

typedef DFS Element;



class GspanProjTrans: public ProjTrans {
public:
	vector<Edge*> pgraph; //projected graph, elements are edges in a data graph that constitute the right-most path
	GspanProjTrans(){};
	GspanProjTrans(int _id, Edge* _e, GspanProjTrans* _prev): ProjTrans(_id){
		if(_prev !=0)
			pgraph = _prev->pgraph; //deep copy
		pgraph.push_back(_e);
	};
};

//pdb to certain pattern
class GspanProjDB: public vector<GspanProjTrans> {
public:
	int sup; // pdb may contain multiple matched instances of the same transaction
	void push (int id, Edge *edge, GspanProjTrans *prev)
	{
		resize (size() + 1);
		GspanProjTrans &d = (*this)[size()-1];
		d.tid = id;
		if(prev!=0)
			d.pgraph = prev->pgraph;
		d.pgraph.push_back(edge);
	}

	//support counting. Each graph will support current pattern only once
	unsigned int support ()
	{
		unsigned int size = 0;
		set<unsigned int> visited;

		for (vector<GspanProjTrans>::iterator cur = begin(); cur != end(); ++cur) {
			int tid = cur->tid;
			if (visited.find(tid) == visited.end()) {
				visited.insert(tid);
				++size;
			}
		}

		sup = size;
		return size;
	}
};



//store all visited vertex and edge. History will be built with a GspanProj and its corresponding GspanTrans object by build()
//used to avoid expanding an already visited edges in the transaction when growing projected transaction into the pdbs of the children
class History: public vector<Edge*> {
private:
	vector<int> edge;
	vector<int> vertex;

public:
	bool hasEdge   (unsigned int id) { return (bool)edge[id]; }
	bool hasVertex (unsigned int id) { return (bool)vertex[id]; }
	void build (GspanTrans *graph, GspanProjTrans *e)
	{
		// first build history
		clear ();
		edge.clear ();
		edge.resize (graph->edge_size());
		vertex.clear ();
		vertex.resize (graph->graph.size());

		if (e) {
			#ifdef USE_ASSERT
			assert((e->pgraph[e->pgraph.size()-1]->id)<edge.size());
			assert((e->pgraph[e->pgraph.size()-1]->from)<vertex.size());
			assert((e->pgraph[e->pgraph.size()-1]->to)<vertex.size());
			#endif

			for(int i = 0; i<e->pgraph.size(); i++){
				push_back (e->pgraph[i]);	// this line eats 8% of overall instructions(!)
				edge[e->pgraph[i]->id] = vertex[e->pgraph[i]->from] = vertex[e->pgraph[i]->to] = 1;
			}
		}
	}
	History() {};
	History (GspanTrans* g, GspanProjTrans *p) { build (g, p); }

};

typedef vector <Edge*> EdgeList;


#endif /* GSPANTRANS_H_ */
