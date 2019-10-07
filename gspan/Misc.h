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

#ifndef MISC_H_
#define MISC_H_
#include <vector>
#include <map>
#include <assert.h>

#include "GspanTrans.h"

/* get_forward_pure ()
 * @e: the last edge in the rightmost path
 * @result: the output
 *
 * get all forward edges that begin from the rightest vertex
*/
bool get_forward_pure (GspanTrans *trans, Edge *e, int minlabel, History& history, EdgeList &result)
{
	result.clear ();
	vector<Vertex>& graph = trans->graph;
	#ifdef USE_ASSERT
	assert (e->to >= 0 && e->to < graph.size ());
	#endif
	/* Walk all edges leaving from vertex e->to.
	 */
	for (Vertex::edge_iterator it = graph[e->to].edge.begin() ;
		it != graph[e->to].edge.end() ; ++it)
	{
		/* -e-> [e->to] -it-> [it->to]
		 */
		#ifdef USE_ASSERT
		assert (it->to >= 0 && it->to < graph.size ());
		#endif
		/*only extend to the vertex which has a larger label than the begin vertex to avoid redundancy.*/
		if (minlabel > graph[it->to].label || history.hasVertex (it->to))
			continue;

		result.push_back (&(*it));
	}

	return (! result.empty());
}

/* get_forward_pure ()
 * 	@e: the path in the rightmost path
 * 	@result: the output
 *
 * 	get all forward edges that start from vertex in the right-most path but the right-most vertex
*/
bool get_forward_rmpath (GspanTrans *trans, Edge *e, int minlabel, History& history, EdgeList &result){
	vector<Vertex>& graph = trans->graph;
	result.clear ();
	#ifdef USE_ASSERT
	assert (e->to >= 0 && e->to < graph.size ());
	assert (e->from >= 0 && e->from < graph.size ());
	#endif
	int tolabel = graph[e->to].label;

	for (Vertex::edge_iterator it = graph[e->from].edge.begin() ;
		it != graph[e->from].edge.end() ; ++it)
	{
		int tolabel2 = graph[it->to].label;
		/*only extend to edges that source vertex's edge in the right-most path has a smaller edge label than the extended edge or they are
		 * 	 equal but the earlier's dest vertex label is smaller to avoid redundancy.*/
		if (e->to == it->to || minlabel > tolabel2 || history.hasVertex (it->to))
			continue;

		if (e->elabel < it->elabel || (e->elabel == it->elabel && tolabel <= tolabel2))
			result.push_back (&(*it));
	}

	return (! result.empty());
};


/* get_forward_root ()
 * 	get all forward edges that start from v
*/
bool get_forward_root (GspanTrans *trans, Vertex &v, EdgeList &result)
{
	vector<Vertex>& g = trans->graph;
	result.clear ();
	for (Vertex::edge_iterator it = v.edge.begin(); it != v.edge.end(); ++it) {
		#ifdef USE_ASSERT
		assert (it->to >= 0 && it->to < g.size ());
		#endif
		if (v.label <= g[it->to].label)
			result.push_back (&(*it));
	}
	return (! result.empty());
}

/* get_backward (graph, e1, e2, history);
*	@e1: the edges in the right-most path but e2
*	@e2: the right-most edge.

*    get all back edges that begin from the right-most vertex
 */
Edge *get_backward (GspanTrans *trans, Edge* e1, Edge* e2, History& history)
{
	if (e1 == e2)
		return 0;
	vector<Vertex>& graph = trans->graph;
	#ifdef USE_ASSERT
	assert (e1->from >= 0 && e1->from < graph.size ());
	assert (e1->to >= 0 && e1->to < graph.size ());
	assert (e2->to >= 0 && e2->to < graph.size ());
	#endif

	for (Vertex::edge_iterator it = graph[e2->to].edge.begin() ;
		it != graph[e2->to].edge.end() ; ++it)
	{
		if (history.hasEdge (it->id))
			continue;
   	   	/*only extend to the vertex that its edge in the right-most path has a smaller edge label than the extended edge
   	   	or their edge's label are the same but dest vertex's label of the earlier is smaller to avoid redundancy.*/
		if ( (it->to == e1->from) && //which means it is a back edge
            (
              (e1->elabel < it->elabel) ||
                (
                  (e1->elabel == it->elabel) &&
                  (graph[e1->to].label <= graph[e2->to].label)
                )
            )
       )
		{
			return &(*it);
		}
	}

	return 0;
}



#endif /* MISC_H_ */
