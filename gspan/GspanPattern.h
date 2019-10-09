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

#ifndef GSPANPATTERN_H_
#define GSPANPATTERN_H_
#include <set>
#include "GspanTrans.h"
#include "Misc.h"

typedef vector<int> RMPath;// the rightmost path of a pattern graph, in inverted order

//forward edge, used for generate children
struct ForwardEdge{
	union{
		int from;
		int fromLabel;
	};
	int elabel;
	int toLabel;

	ForwardEdge(int from,int e, int to): from(from), elabel(e), toLabel(to) {};

	friend bool operator<(const ForwardEdge& e1, const ForwardEdge& e2)
	{
	    if(e1.from<e2.from)	return true;
	    if((e1.from == e2.from) && e1.elabel < e2.elabel) return true;
	    if((e1.from == e2.from) && (e1.elabel == e2.elabel)&&e1.toLabel < e2.toLabel) return true;
	    return false;
	}

	friend bool operator == (const ForwardEdge &e1, const ForwardEdge &e2)
	{
		return (e1.from == e2.from && e1.toLabel == e2.toLabel && e1.elabel == e2.elabel);
	}
};

struct cmp_reversefrom{
	bool operator()(const ForwardEdge &e1, const ForwardEdge &e2)const{
	    if(e1.from>e2.from)	return true;
	    if((e1.from == e2.from) && e1.elabel < e2.elabel) return true;
	    if((e1.from == e2.from) && (e1.elabel == e2.elabel)&&e1.toLabel < e2.toLabel) return true;
	    return false;
	}
};

//back edge, used for generate children
struct BackEdge {
	int to;
	int elabel;

	BackEdge(int to, int e): to(to), elabel(e) {};

	friend bool operator<(const BackEdge& e1, const BackEdge& e2)
	{
	    if(e1.to<e2.to)	return true;
	    else return e1.elabel < e2.elabel;
	}
};


typedef map<ForwardEdge, GspanProjDB>					forwardEdge_Projected; // to store all forward edges expansion and their projected database, only use in first round.
typedef map<ForwardEdge, GspanProjDB,cmp_reversefrom>	forwardEdge_Projected2; /// to store all forward edges expansion and their projected database.
typedef forwardEdge_Projected::iterator						forwardProjected_iter;
typedef forwardEdge_Projected2::iterator					forwardProjected_iter2;

typedef map<BackEdge, GspanProjDB>				backEdge_Projected;// to store all forward edges expansion and their projected database.
typedef backEdge_Projected::iterator				backProjected_iter;


class GspanPattern:public Pattern{
public:
	vector<Element> items; //vector of DFS code quintuple
	vector<GspanProjTrans> pdb;

	RMPath rmpath;
	int sup; // pdb may contain multiple matched instances of the same transaction

	/*build the right-most path for subgraph
	 */
	const RMPath& buildRMPath (){
		rmpath.clear ();

		int old_from = -1;

		for (int i = items.size() - 1 ; i >= 0 ; --i) {
			if (items[i].from < items[i].to && // forward
					(rmpath.empty() || old_from == items[i].to))
			{
				rmpath.push_back (i);
				old_from = items[i].from;
			}
		}

		return rmpath;
	};

	/* Convert current DFS code into a graph.
	 */
	bool toGraph (GspanTrans &trans){
		vector<Vertex>& g = trans.graph;
		g.clear ();

		for (vector<Element>::iterator it = items.begin(); it != items.end(); ++it) {
			g.resize (max (it->from, it->to) + 1);

			if (it->fromlabel != -1)
				g[it->from].label = it->fromlabel;
			if (it->tolabel != -1)
				g[it->to].label = it->tolabel;

			g[it->from].push (it->from, it->to, it->elabel);
			if (directed == false)
				g[it->to].push (it->to, it->from, it->elabel);
		}

		trans.buildEdge ();

		return (true);
	};

	void push (int from, int to, int fromlabel, int elabel, int tolabel)
	{
		items.resize (items.size() + 1); //create one more space at the list tail
		Element &d = items.back(); //get the last element just created

		//set the fields
		d.from = from;
		d.to = to;
		d.fromlabel = fromlabel;
		d.elabel = elabel;
		d.tolabel = tolabel;
	}

	void clear(){
		items.clear();
		items.shrink_to_fit();
		pdb.clear();
		pdb.shrink_to_fit();
	}

	void print(ostream& fout){
		GspanTrans* t= new GspanTrans;
		if(toGraph(*t)){
			fout << "t ";
			#ifdef USE_ID
			fout << "# " << ID++;
			#endif
			fout << " * "<< sup << endl;
			t->write(fout);
		};
		delete t;
	}
};


#endif /* GSPANPATTERN_H_ */
