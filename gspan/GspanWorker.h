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

#ifndef GSPANWORKER_H_
#define GSPANWORKER_H_
#include <iostream>
#include <fstream>
#include <set>

#include "GspanTask.h"

class GspanWorker: public Worker<GspanTask> {
public:
	int trans_cnt;// to used for adding IDs to transactions when loading data

	GspanWorker(const char *infile, const char *outfolder = "outputs"): Worker(infile, outfolder), trans_cnt(0){
	}

	~GspanWorker(){
		for(int i = 0; i<TransDB().size(); i++)
			delete TransDB()[i];
	}


	virtual int getNextTrans(vector<TransT>& transDB){
		GspanTrans* g = new GspanTrans(trans_cnt++);
		g->read (fd);
		if (g->graph.empty()){
			delete g;
			return 0;
		}

		TransDB().push_back (g);
		return 1;
	}

	virtual void setRoot(stack<GspanTask*>& task_queue){
		if (vLabel_patt == true) {
			/* Do single node handling, as the normal gspan DFS code based processing
			 * cannot find subgraphs of size |subg|==1.  Hence, we find frequent node
			 * labels explicitly.
			 */
			map<unsigned int, unsigned int> singleVertexLabel;
			vector<map<unsigned int, unsigned int> > VertexOfThread(THREADS);
			#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
			for (unsigned int id = 0; id < TransDB().size(); ++id) {
				int thread_id = omp_get_thread_num();
				GspanTrans* trans = TransDB()[id];
				set<int> labelSet;
				for (unsigned int nid = 0 ; nid < trans->graph.size() ; ++nid) {
					labelSet.insert(trans->graph[nid].label);
				}
				for(set<int>::iterator it=labelSet.begin(); it != labelSet.end(); it++)
					VertexOfThread[thread_id][*it]++;
			}
			for(int i = 0; i < THREADS; i++){
				map<unsigned int, unsigned int>& cur = VertexOfThread[i];
				for(map<unsigned int, unsigned int>::iterator it = cur.begin(); it!=cur.end(); it++)
					singleVertexLabel[it->first] += it->second;
			}

			set<int> frequent_set;

			for (map<unsigned int, unsigned int>::iterator it =
				singleVertexLabel.begin () ; it != singleVertexLabel.end () ; ++it)
			{
				if (it->second < minsup)
					continue;

				unsigned int frequent_label = it->first;
				frequent_set.insert(frequent_label);
				/* Found a frequent node label, report it.
				 */
				GspanTrans g;
				g.graph.resize (1);
				g.graph[0].label = frequent_label;
				fouts[0] << "t ";
				#ifdef USE_ID
				fouts[0] << "# " <<++ID;
				#endif
				fouts[0] << " * "<< it->second << endl;
				g.write(fouts[0]);

			}

			//delete infrequent edges and build edges id from each graph
			#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
			for (unsigned int id = 0; id < TransDB().size(); ++id) {
				GspanTrans* trans = TransDB()[id];
				for (vector<Vertex>::iterator it1 = trans->graph.begin(); it1 != trans->graph.end();) {
					if(frequent_set.find(it1->label) == frequent_set.end()){
						it1->edge.clear();
					}
					else{
						vector<Edge> new_edgeList;
						for(Vertex::edge_iterator it2 = it1->edge.begin(); it2 != it1->edge.end();){
							if(frequent_set.find(trans->graph[it2->from].label) != frequent_set.end()){
								new_edgeList.push_back(*it2);
							}
							it2++;
						}
						it1->edge.swap(new_edgeList);
					}
					it1++;
				}
				trans->buildEdge();
			}
		}

		//used to store all frequent edges.
		forwardEdge_Projected candidates;

		if(TransDB().size() <= tauDB_omp){
			for (unsigned int id = 0; id < TransDB().size(); ++id) {
				GspanTrans *g = TransDB()[id];
				for (unsigned int from = 0; from < g->graph.size() ; ++from) {
					EdgeList edges;
					if (get_forward_root (g, g->graph[from], edges)) {
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
							ForwardEdge edge(g->graph[from].label,(*it)->elabel,g->graph[(*it)->to].label);
							candidates[edge].push (id, *it, 0);
						}
					}
				}
			}
		}
		else{
			// parallel-for, set the array of candidatesOfThread
			vector<forwardEdge_Projected> candidatesOfThread(THREADS);
			#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
			for (unsigned int id = 0; id < TransDB().size(); ++id) {
				int thread_id = omp_get_thread_num();
				GspanTrans *g = TransDB()[id];
				for (unsigned int from = 0; from < g->graph.size() ; ++from) {
					EdgeList edges;
					if (get_forward_root (g, g->graph[from], edges)) {
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
							ForwardEdge edge(g->graph[from].label,(*it)->elabel,g->graph[(*it)->to].label);
							candidatesOfThread[thread_id][edge].push (id, *it, 0);
						}
					}
				}
			}
			// merge candidatesOfThread elements
			for(int i = 0; i < THREADS; i++){
				forwardEdge_Projected& candidates_i = candidatesOfThread[i];
				for(forwardProjected_iter it = candidates_i.begin(); it!= candidates_i.end(); it++){
					GspanProjDB& pdb_i = it->second;
					GspanProjDB& pdb = candidates[it->first];
					pdb.insert(pdb.end(), pdb_i.begin(), pdb_i.end());
					pdb_i.clear(); // to release memory space timely
				}
				candidates_i.clear(); // to release memory space timely
			}

			for(forwardProjected_iter it = candidates.begin(); it!= candidates.end(); ){
				GspanProjDB& pdb = it->second;
				int sup = pdb.support();
				if (sup < minsup) {
					forwardProjected_iter tmp = it;
					++tmp;
					candidates.erase(it);
					it = tmp;
				} else {
					++it;
				}
			}
		}

		//start root task
		for (forwardProjected_iter fromlabel = candidates.begin() ;
					fromlabel != candidates.end() ; ++fromlabel){
			GspanTask* rootTask = new GspanTask;
			ForwardEdge edge = fromlabel->first;
			GspanPattern& curPat = rootTask->pat;
			curPat.push (0, 1, edge.fromLabel, edge.elabel, edge.toLabel);
			//swap the memory of projected database into new pattern.
			curPat.pdb.swap(fromlabel->second);
			curPat.sup = fromlabel->second.sup;
			task_queue.push(rootTask);
		}
	}
};
#endif /* GSPANWORKER_H_ */

