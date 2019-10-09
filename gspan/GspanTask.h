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

#ifndef GSPANTASK_H_
#define GSPANTASK_H_
#include "GspanPattern.h"
#include "omp.h"


struct ChildrenT{
	forwardEdge_Projected2 new_fwd_root;
	forwardProjected_iter2 it1;

	backEdge_Projected new_bck_root;
	backProjected_iter it2;
};

class GspanTask: public Task<GspanPattern, ChildrenT, GspanTrans*> {
public:
	int minlabel; //min vertex label in current pattern graph
	int maxtoc; //the dfs id of the right-most vertex

	GspanPattern	MIN_PATTERN;
	GspanTrans   MIN_GRAPH;

	bool project_is_min (GspanProjDB &projected)
	{
		const RMPath& rmpath = MIN_PATTERN.buildRMPath ();
		int minlabel         = MIN_PATTERN.items[0].fromlabel;
		int maxtoc           = MIN_PATTERN.items[rmpath[0]].to;

		/*===========================begin===========================
		*enum all back edge, forward edge and find the minimum edge. Then store the minimum edge into MIN_PATTERN
		*/
		//first, back edge
		{
			backEdge_Projected root;
			bool flg = false;


			for (int i = rmpath.size()-1; ! flg  && i >= 1; --i) {
				for (unsigned int n = 0; n < projected.size(); ++n) {
					GspanProjTrans *cur = &projected[n];
					History history (&MIN_GRAPH, cur);
					Edge *e = get_backward (&MIN_GRAPH, history[rmpath[i]], history[rmpath[0]], history);
					if (e) {
						BackEdge edge(MIN_PATTERN.items[rmpath[i]].from,e->elabel);
						root[edge].push (0, e, cur);
						flg = true;
					}
				}
			}

			if (flg) {
				backProjected_iter to = root.begin();
				BackEdge edge = to->first;
				MIN_PATTERN.push (maxtoc, edge.to, -1, edge.elabel, -1);
				if (pat.items[MIN_PATTERN.items.size()-1] != MIN_PATTERN.items [MIN_PATTERN.items.size()-1]) return false;
				return project_is_min (to->second);
			}
		}

		//then forward edge
		{
			bool flg = false;
			forwardEdge_Projected root;
			EdgeList edges;

			for (unsigned int n = 0; n < projected.size(); ++n) {
				GspanProjTrans *cur = &projected[n];
				History history (&MIN_GRAPH, cur);
				if (get_forward_pure (&MIN_GRAPH, history[rmpath[0]], minlabel, history, edges)) {
					flg = true;
					for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
						ForwardEdge edge(maxtoc,(*it)->elabel,MIN_GRAPH.graph[(*it)->to].label);
						root[edge].push(0, *it, cur);
					}
				}
			}

			for (int i = 0; ! flg && i < (int)rmpath.size(); ++i) {
				for (unsigned int n = 0; n < projected.size(); ++n) {
					GspanProjTrans *cur = &projected[n];
					History history (&MIN_GRAPH, cur);
					if (get_forward_rmpath (&MIN_GRAPH, history[rmpath[i]], minlabel, history, edges)) {
						flg = true;
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
							ForwardEdge edge(MIN_PATTERN.items[rmpath[i]].from,(*it)->elabel,MIN_GRAPH.graph[(*it)->to].label);
							root[edge].push(0, *it, cur);
						}
					}
				}
			}

			if (flg) {
				forwardProjected_iter from  = root.begin();
				ForwardEdge edge = from->first;
				MIN_PATTERN.push (edge.from, maxtoc + 1, -1, edge.elabel, edge.toLabel);
				if (pat.items[MIN_PATTERN.items.size()-1] != MIN_PATTERN.items [MIN_PATTERN.items.size()-1]) return false;
				return project_is_min (from->second);
			}
		}
		/*===========================end===========================*/

		return true;
	}

	/**get the origin subgraph from DFS code, then regenerate the canonical DFS code
	 *  for the subgraph, and judge whether current DFS code equals to the canonical DFS***
	*/
	bool is_min ()
	{

		if (pat.items.size() == 1)
			return (true);

		pat.toGraph (MIN_GRAPH);
		MIN_PATTERN.items.clear ();

		forwardEdge_Projected root;
		EdgeList           edges;

		for (unsigned int from = 0; from < MIN_GRAPH.graph.size() ; ++from)
			if (get_forward_root (&MIN_GRAPH, MIN_GRAPH.graph[from], edges))
				for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
					ForwardEdge edge(MIN_GRAPH.graph[from].label,(*it)->elabel,MIN_GRAPH.graph[(*it)->to].label);
					root[edge].push(0, *it, 0);
				}
		forwardProjected_iter fromlabel = root.begin();
		//the first edge in root is the minimum edge, so push it into MIN_PATTERN
		ForwardEdge edge = fromlabel->first;

		MIN_PATTERN.push (0, 1, edge.fromLabel, edge.elabel, edge.toLabel);

		return (project_is_min (fromlabel->second));
	}


	//infrequent pattern should have been pruned when generating child patterns from its parent
	virtual bool pre_check(ostream& fout){
		pat.print(fout);
		return true;
	}

	bool needSplit(){
		if(pat.pdb.size() > tauDB_singlethread)
			return true;
		return false;
	};

	/*
	 * enum the children of an existed pattern, first back edge, then forward edge.
	 */
	virtual void setChildren(ChildrenT& children){
		// We just output a frequent subgraph.  As it is frequent enough, so might be its (n+1)-extension-graphs, hence we enumerate them all.
		vector<Element>& DFS_CODE = pat.items;
		const RMPath &rmpath = pat.buildRMPath ();
		minlabel = DFS_CODE[0].fromlabel;
		maxtoc = DFS_CODE[rmpath[0]].to; //the right-most vertex
		vector<GspanProjTrans>& projected = pat.pdb;

		//Enumerate all possible one edge extensions of the current substructure.
		if(projected.size() <= tauDB_omp){
			for (unsigned int n = 0; n < projected.size(); ++n) {
				EdgeList edges;
				unsigned int id = projected[n].tid;
				GspanProjTrans *cur = &projected[n];
				History history (TransDB()[id], cur);


				// backward
				for (int i = (int)rmpath.size()-1; i >= 1; --i) {
					/*
					 * rmpath[0]: the last edge id in the rmpath
					 */
					Edge *e = get_backward (TransDB()[id], history[rmpath[i]], history[rmpath[0]], history);
					if (e){ //NULL if history[rmpath[i]] and history[rmpath[0]] have no edge or the edge is redundancy
						BackEdge edge(DFS_CODE[rmpath[i]].from,e->elabel);
						children.new_bck_root[edge].emplace_back (cur->tid, e, cur);
					}
				}

				// pure forward: from rightmost vertex
				if (get_forward_pure (TransDB()[id], history[rmpath[0]], minlabel, history, edges)) // edges are set here
					for (EdgeList::iterator it = edges.begin(); it != edges.end(); ++it){
						ForwardEdge edge(maxtoc, (*it)->elabel,TransDB()[id]->graph[(*it)->to].label);
						children.new_fwd_root[edge].emplace_back (cur->tid, *it, cur);
					}

				// backtracked forward
				for (int i = 0; i < (int)rmpath.size(); ++i)
					if (get_forward_rmpath (TransDB()[id], history[rmpath[i]], minlabel, history, edges))
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
							ForwardEdge edge(DFS_CODE[rmpath[i]].from,(*it)->elabel,TransDB()[id]->graph[(*it)->to].label);
							children.new_fwd_root[edge].emplace_back (cur->tid, *it, cur);
						}
			}
		}
		else{
			vector<ChildrenT> childrenOfThread(THREADS);
			#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
			for (unsigned int n = 0; n < projected.size(); ++n){
				EdgeList edges;
				int thread_id = omp_get_thread_num();
				unsigned int id = projected[n].tid;
				GspanProjTrans *cur = &projected[n];
				History history (TransDB()[id], cur);

				// backward
				for (int i = (int)rmpath.size()-1; i >= 1; --i) {
					/*
					 * rmpath[0]: the last edge id in the rmpath
					 */
					Edge *e = get_backward (TransDB()[id], history[rmpath[i]], history[rmpath[0]], history);
					if (e){
						BackEdge edge(DFS_CODE[rmpath[i]].from,e->elabel);
						childrenOfThread[thread_id].new_bck_root[edge].emplace_back (cur->tid, e, cur);
					}
				}

				// pure forward
				if (get_forward_pure (TransDB()[id], history[rmpath[0]], minlabel, history, edges))
					for (EdgeList::iterator it = edges.begin(); it != edges.end(); ++it){
						ForwardEdge edge(maxtoc,(*it)->elabel,TransDB()[id]->graph[(*it)->to].label);
						childrenOfThread[thread_id].new_fwd_root[edge].emplace_back (cur->tid, *it, cur);
					}

				// backtracked forward
				for (int i = 0; i < (int)rmpath.size(); ++i)
					if (get_forward_rmpath (TransDB()[id], history[rmpath[i]], minlabel, history, edges))
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
							ForwardEdge edge(DFS_CODE[rmpath[i]].from,(*it)->elabel,TransDB()[id]->graph[(*it)->to].label);
							childrenOfThread[thread_id].new_fwd_root[edge].emplace_back (cur->tid, *it, cur);
						}
			}

			// merge childrenOfThread elements into children
			for(int i = 0; i < THREADS; i++){
				ChildrenT& children_i = childrenOfThread[i];
				backEdge_Projected& back_root = children_i.new_bck_root;
				forwardEdge_Projected2& forward_root = children_i.new_fwd_root;
				for(backEdge_Projected::iterator it = back_root.begin(); it!= back_root.end(); it++){
					GspanProjDB& pdb_i = it->second;
					GspanProjDB& pdb = children.new_bck_root[it->first];
					pdb.insert(pdb.end(), pdb_i.begin(), pdb_i.end());
					pdb_i.clear(); // to release memory space timely
				}
				children_i.new_bck_root.clear(); // to release memory space timely
				for(forwardEdge_Projected2::iterator it = forward_root.begin(); it!= forward_root.end(); it++){
					GspanProjDB& pdb_i = it->second;
					GspanProjDB& pdb = children.new_fwd_root[it->first];
					pdb.insert(pdb.end(), pdb_i.begin(), pdb_i.end());
					pdb_i.clear(); // to release memory space timely
				}
				children_i.new_fwd_root.clear(); // to release memory space timely
			}
		}

		for(forwardProjected_iter2 it = children.new_fwd_root.begin(); it != children.new_fwd_root.end();){
			GspanProjDB& pdb = it->second;
			int sup = pdb.support();
			if (sup < minsup) {
				forwardProjected_iter2 tmp = it;
				++tmp;
				children.new_fwd_root.erase(it);
				it = tmp;
			} else {
				++it;
			}
		}
		for(backProjected_iter it = children.new_bck_root.begin(); it != children.new_bck_root.end();){
			GspanProjDB& pdb = it->second;
			int sup = pdb.support();
			if (sup < minsup) {
				backProjected_iter tmp = it;
				++tmp;
				children.new_bck_root.erase(it);
				it = tmp;
			} else {
				++it;
			}
		}

		children.it2 = children.new_bck_root.begin();
		children.it1 = children.new_fwd_root.begin();

	}

	/*
	 * wrap a pattern and its project transactions into a new task
	 * e: the child append to previous pattern
	 * proj: the project transactions of new pattern
	 */
	Task* project(const Element& e, GspanProjDB& proj){
		GspanTask* newTask = new GspanTask;
		GspanPattern& newPat = newTask->pat;

		//new pattern is old pattern plus new element
		newPat.items.assign(pat.items.begin(),pat.items.end());
		newPat.push (e.from,e.to,e.fromlabel,e.elabel,e.tolabel);

		//swap the memory of projected database into new pattern.
		newPat.pdb.swap(proj);
		newPat.sup = proj.sup;

		return newTask;
	};

	virtual Task* get_next_child(){
		while(children.it2!=children.new_bck_root.end()){
			BackEdge edge = children.it2->first;
			Element e(maxtoc, edge.to, -1, edge.elabel, -1);
			GspanProjDB& pdb = children.it2->second;
			Task* newTask = this->project(e, pdb);
			children.it2++;
			//here the pattern and transaction are both ready, so delete infrequent and not canonical pattern here
			if(!((GspanTask*)newTask)->is_min()) {
				delete newTask;
				continue;
			}
			return newTask;
		}
		while(children.it1!=children.new_fwd_root.end()){
			ForwardEdge edge = children.it1->first;

			Element e (edge.from, maxtoc+1, -1, edge.elabel, edge.toLabel);
			GspanProjDB& pdb = children.it1->second;
			Task* newTask = this->project(e, pdb);
			children.it1++;
			if(!((GspanTask*)newTask)->is_min()) {
				delete newTask;
				continue;
			}
			return newTask;
		}
		return 0;
	}

};


#endif /* GSPANTASK_H_ */
