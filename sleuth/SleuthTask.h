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

#ifndef SLEUTHTASK_H_
#define SLEUTHTASK_H_
#include <map>

#include "Misc.h"
class SleuthTask: public Task<SleuthPattern, ChildrenT, SleuthTrans*> {
public:
	int iter;
	ostream* ostr;

	ChildrenT::iterator it;

	virtual bool pre_check(ostream& fout){
		ostr = &fout; // to track fout so that it can be used in setChildren
		return true;
	}

	virtual bool needSplit(){
		if(pat.total_tlist > tauDB_singlethread) return true;
		return false;
	}

	virtual void setChildren(ChildrenT& children) { // implementing Fig 9 of the Sleuth paper
		//ni and nj is the two elements that do the "scope lists join", which mentioned in zaki's work.
		vector<Element*>::iterator ni, nj;
		//"out" means the nodes that nj is the cousin extension of ni, while "ins" means node that nj is the child extension of ni
		Element *ins, *outs;

		if (prune_type == noprune)
			pat.sort_nodes(); //doesn't work with pruning // it should be executed

		for (ni = pat.nlist().begin(); ni != pat.nlist().end(); ++ni) {
			SleuthPattern *npat = new SleuthPattern;
			npat->set_prefix(pat.prefix(), *(*ni));

			//check current pattern
			bool res;
			res = check_canonical(npat->prefix(), npat->prefix().size() - 1);
			if (!res) {
				delete npat;
				continue;
			}
			if (output) {
				*ostr << "ID " << ID++ << ":";
				npat->print(*ostr);
				*ostr << *(*ni);
			}

			for (nj = pat.nlist().begin(); nj != pat.nlist().end(); ++nj) {
				//to avoid duplicated tree
				if ((*ni)->pos < (*nj)->pos)
					continue;
				ins = outs = NULL;
				if ((*ni)->pos > (*nj)->pos) {
					//only do cousin extension
					outs = test_node(iter, npat, (*nj)->val, (*nj)->pos);
				} else { // only "equal" case
					//both cousin extension and child extension
					outs = test_node(iter, npat, (*nj)->val, (*nj)->pos);
					ins = test_node(iter, npat, (*nj)->val,
							npat->prefix().size() - 1);
				}

				//generate projected database in tlist for "inside" node and "outside" node
				if (ins || outs)
					//process parallel
					get_intersect(&(*ni)->tlist, &(*nj)->tlist, ins, outs);

				//delete infrequent candidates
				if (outs) {
					if (notfrequent(*outs, iter))
						delete outs;
					else {
						children[*ni].push_back(outs);
						pat.total_tlist += outs->tlist.size();
					}
				}
				if (ins) {
					if (notfrequent(*ins, iter))
						delete ins;
					else {
						children[*ni].push_back(ins);
						pat.total_tlist += ins->tlist.size();
					}
				}
			}
			delete npat;
		}

		it = children.begin();
	}

	Task* project(const Element& ni, vector<Element *>& nj) {
		SleuthTask *t = new SleuthTask;
		SleuthPattern &p = t->pat;
		p.set_prefix(pat.prefix(), ni);
		p._nodelist.swap(nj);
		if (!p.nlist().empty()) {
			if (prune_type == prune)
				FK.add(iter, &p);
		}

		t->iter = iter + 1;

		return t;
	}

	virtual Task* get_next_child(){
		if(it!=children.end()){
			Task* newTask = this->project(*(it->first), it->second);
			it++;
			return newTask;
		}
		return 0;
	}
};

#endif /* TASK_H_ */
