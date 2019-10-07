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

#ifndef PREFIXSPAN_PREFIXSPAN_H_
#define PREFIXSPAN_PREFIXSPAN_H_

#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <strstream>

#include "Trans.h"
#include "Pattern.h"
#include "Task.h"
#include "Worker.h"

#include "omp.h"

using namespace std;


typedef int Element;
typedef vector<int> sequence;

//---------------------------

class ProjSeq: public ProjTrans{
public:
	int pos;
	ProjSeq(){};
	ProjSeq(int _id, int _pos): ProjTrans(_id), pos(_pos){};
};

//---------------------------

class Seq: public Trans{
public:
	sequence seq;
	Seq(){};
	Seq(int _tid): Trans(_tid){};
};

//---------------------------

typedef vector<ProjSeq> seqPDB;

class PrefixPattern: public Pattern {
public:
	sequence items; // elements of a sequence (in order)
	vector<ProjSeq>* projDB; // should be set from outside, usually passed in from children_map

	~PrefixPattern(){delete projDB;}

	virtual void print(ostream& fout){
		for(vector<Element>::iterator it = items.begin(); it!=items.end(); it++) fout << *it << " ";
		fout << " |PDB| = " << projDB->size() << endl;
	}
};

//---------------------------

typedef map<Element, seqPDB*> ChildrenT;

class PrefixTask: public Task<PrefixPattern, ChildrenT, Seq>{
public:
	ChildrenT::iterator it;

	virtual bool needSplit(){
		if(pat.projDB->size() > tauDB_singlethread) return true;
		return false;
	};

	// with omp, branching on tauDB_omp
	virtual void setChildren(ChildrenT& children) {
		//get current projected DB (to set children's projected DBs)
		vector<ProjSeq>& oldPDB = *(pat.projDB);

		if(oldPDB.size() <= tauDB_omp)
		{
			for (int i=0; i<oldPDB.size(); i++) {
				int pos = oldPDB[i].pos;
				unsigned int id = oldPDB[i].tid;
				sequence& seq = TransDB()[id].seq;
				unsigned int size = seq.size();

				set<int> tmp; //to filter out labels that already occur in a previous position in the current seq
				//continue scanning elements from "pos"
				for (unsigned int j = pos + 1; j < size; j++) {
					//avoid checking repeated label
					int item = seq[j];
					if(item == -1) continue;
					if (tmp.find(item) != tmp.end()) continue;
					tmp.insert(item);
					//add current seq to children's PDB
					ChildrenT::iterator it = children.find(item);
					if(it == children.end()){
						vector<ProjSeq>* pdb = new vector<ProjSeq>;
						pdb->emplace_back(id, j);
						children[item] = pdb;
					}
					else{
						vector<ProjSeq>* pdb = it->second;
						pdb->emplace_back(id, j);
					}
				}
			}
		}
		else
		{
			vector<ChildrenT> childrenOfThread(THREADS);
			// parallel-for, set the array of childrenOfThread
			#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
			for (int i=0; i<oldPDB.size(); i++) {
				int thread_id = omp_get_thread_num();
				ChildrenT& child_list = childrenOfThread[thread_id];
				//------
				int pos = oldPDB[i].pos;
				unsigned int id = oldPDB[i].tid;
				sequence& seq = TransDB()[id].seq;
				unsigned int size = seq.size();

				set<int> tmp; //to filter out labels that already occur in a previous position in the current seq
				//continue scanning elements from "pos"
				for (unsigned int j = pos + 1; j < size; j++) {
					//avoid checking repeated label
					int item = seq[j];
					if(item == -1) continue;
					if (tmp.find(item) != tmp.end()) continue;
					tmp.insert(item);
					//add current seq to children's PDB
					ChildrenT::iterator it = child_list.find(item);
					if(it == child_list.end()){
						vector<ProjSeq>* pdb = new vector<ProjSeq>;
						pdb->emplace_back(id, j);
						child_list[item] = pdb;
					}
					else{
						vector<ProjSeq>* pdb = it->second;
						pdb->emplace_back(id, j);
					}
				}
			}
			// merge childrenOfThread elements into children
			for(int i = 0; i < THREADS; i++){
				ChildrenT& children_i = childrenOfThread[i];
				for(ChildrenT::iterator it = children_i.begin(); it!= children_i.end(); it++){
					Element label = it->first;
					seqPDB* pdb_i = it->second;
					ChildrenT::iterator it2 = children.find(label);
					if(it2 == children.end()){
						vector<ProjSeq>* pdb = new vector<ProjSeq>;
						pdb->swap(*pdb_i);
						children[label] = pdb;
					}
					else
					{
						vector<ProjSeq>* pdb = it2->second;
						pdb->insert(pdb->end(), pdb_i->begin(), pdb_i->end());
						pdb_i->clear(); // to release memory space timely
					}
				}
				children_i.clear(); // to release memory space timely
			}
		}

		//delete child patterns with PDBs not big enough
		for (ChildrenT::iterator it = children.begin(); it != children.end();)
		{
			vector<ProjSeq>* pdb = it->second;
			if (pdb->size() < minsup) {
				ChildrenT::iterator tmp = it;
				++tmp;
				delete it->second;
				children.erase(it);
				it = tmp;
			} else {
				++it;
			}
		}

		// set iterator as the first element
		it = children.begin();
	}

	Task* project(const Element& e, vector<ProjSeq>* projDB){
		Task* newT = new PrefixTask;
		PrefixPattern& newPat = newT->pat;
		newPat.items.assign(pat.items.begin(), pat.items.end());
		newPat.items.push_back(e);
		newPat.projDB = projDB;
		return newT;
	}

	virtual bool pre_check(ostream& fout){
		pat.print(fout);
		return true;
	}

	virtual Task* get_next_child(){
		if(it != children.end()){
			Task* newTask = this->project(it->first, it->second);
			it++;
			return newTask;
		}
		return 0;
	}
};

class PrefixWorker:public Worker<PrefixTask> {
public:
	int trans_cnt; // to used for adding IDs to transactions when loading data

	PrefixWorker(const char *infile, const char *outfolder = "outputs"): Worker(infile, outfolder), trans_cnt(0)
	{}

	//set map[label] = count
	virtual int getNextTrans(vector<TransT>& transDB){
		string line;
	    int item;
		if(getline (fd, line)){
			TransDB().resize(TransDB().size()+1);
	    	Seq& tmp = TransDB().back();
	    	tmp.tid = trans_cnt++;
	        istrstream istrs ((char *)line.c_str());
	        while (istrs >> item) tmp.seq.push_back (item);
	    	return 1;
	    }

		return 0;
	}

	virtual void setRoot(stack<PrefixTask*>& task_queue){
		ChildrenT children;

		if(TransDB().size() <= tauDB_omp)
		{
			for (int i=0; i<TransDB().size(); i++) {

				sequence& seq = TransDB()[i].seq;
				set<int> tmp; //to filter out labels that already occur in a previous position in the current seq

				for (unsigned int j = 0; j < seq.size(); j++) {
					//avoid checking repeated label
					int item = seq[j];
					if (tmp.find(item) != tmp.end()) continue;
					tmp.insert(item);
					//add current seq to children's PDB
					ChildrenT::iterator it = children.find(item);
					if(it == children.end()){
						vector<ProjSeq>* pdb = new vector<ProjSeq>;
						pdb->emplace_back(i, j);
						children[item] = pdb;
					}
					else{
						vector<ProjSeq>* pdb = it->second;
						pdb->emplace_back(i, j);
					}
				}
			}
		}
		else
		{
			vector<ChildrenT> childrenOfThread(THREADS);
			// parallel-for, set the array of childrenOfThread
			#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
			for (int i=0; i<TransDB().size(); i++) {
				int thread_id = omp_get_thread_num();
				ChildrenT& child_list = childrenOfThread[thread_id];
				//------
				sequence& seq = TransDB()[i].seq;
				set<int> tmp; //to filter out labels that already occur in a previous position in the current seq

				for (unsigned int j = 0; j < seq.size(); j++) {
					//avoid checking repeated label
					int item = seq[j];
					if (tmp.find(item) != tmp.end()) continue;
					tmp.insert(item);
					//add current seq to children's PDB
					ChildrenT::iterator it = child_list.find(item);
					if(it == child_list.end()){
						vector<ProjSeq>* pdb = new vector<ProjSeq>;
						pdb->emplace_back(i, j);
						child_list[item] = pdb;
					}
					else{
						vector<ProjSeq>* pdb = it->second;
						pdb->emplace_back(i, j);
					}
				}
			}
			// merge childrenOfThread elements into children
			for(int i = 0; i < THREADS; i++){
				ChildrenT& children_i = childrenOfThread[i];
				for(ChildrenT::iterator it = children_i.begin(); it!= children_i.end(); it++){
					Element label = it->first;
					seqPDB* pdb_i = it->second;
					ChildrenT::iterator it2 = children.find(label);
					if(it2 == children.end()){
						vector<ProjSeq>* pdb = new vector<ProjSeq>;
						pdb->swap(*pdb_i);
						children[label] = pdb;
					}
					else
					{
						vector<ProjSeq>* pdb = it2->second;
						pdb->insert(pdb->end(), pdb_i->begin(), pdb_i->end());
						pdb_i->clear(); // to release memory space timely
					}
				}
				children_i.clear(); // to release memory space timely
			}
		}

		set<int> frquent_labels;
		//delete child patterns with PDBs not big enough
		for (ChildrenT::iterator it = children.begin(); it != children.end();)
		{
			vector<ProjSeq>* pdb = it->second;
			if (pdb->size() < minsup) {
				ChildrenT::iterator tmp = it;
				++tmp;
				delete it->second;
				children.erase(it);
				it = tmp;
			} else {
				PrefixTask *t = new PrefixTask;
				t->pat.items.push_back(it->first);
				t->pat.projDB = it->second;
				task_queue.push(t);
				frquent_labels.insert(it->first);
				++it;
			}
		}

		//make infrequent items invalid
		for(int i = 0; i<TransDB().size(); i++){
			for(sequence::iterator it = TransDB()[i].seq.begin(); it != TransDB()[i].seq.end();){
				if(frquent_labels.find(*it) == frquent_labels.end()){
					*it = -1;
				}
				it++;
			}
		}
	}
};

#endif /* PREFIXSPAN_PREFIXSPAN_H_ */
