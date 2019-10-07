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

#ifndef __sleuth_misc_h
#define __sleuth_misc_h
#include "Hashtable.h"
#include "assert.h"
FreqHT FK;

#define ChildrenT map<Element*, vector<Element*> >

struct TransBlock{
	int st1;
	int st2;
	int end1;
	int end2;
	bool ins_flag; // whether inside or outside
	bool f2;

	TransBlock(int i1, int i2, int e1, int e2, int flag, int _f2): st1(i1), st2(i2), end1(e1), end2(e2), ins_flag(flag), f2(_f2){}
};

//check where a element is frequent
bool notfrequent(Element &n, int iter) {
	if(mine_induced &&  n.isup >= minsup) return false;
	else if(!mine_induced && n.sup >= minsup) return false;

	return true;
}

/*if a tree if canonical, each level of the tree follows certain orders.
 * cand is the full tree, tpos is pos of root of current rightmost subtree
 */
bool check_canonical(vector<int> &cand, int tpos) {
	if (mine_ordered)
		return true;

	//only have to check subtrees on rightmost path, with their immediate siblings to the left, and make sure code is smaller or equal
	bool first;
	int i, parent = 0, sibling;
	int scnt = 0;

	//base condition for stopping recursion, the root has no sibling
	if (tpos == 0) {
		return true;
	}

	//find the pos of parent and immediate left sibling of tpos under parent
	first = true;
	sibling = -1; //not found!
	for (i = tpos - 1; i >= 0; i--) {
		if (cand[i] == BranchIt)
			scnt++;
		else
			scnt--;
		if (first && scnt == 0) {
			sibling = i;
			first = false;
		}

		if (scnt < 0) { // parent is before all siblings
			parent = i;
			break;
		}
	}


	if (sibling == -1) { // not found!
		return check_canonical(cand, parent);
	} else {
		//compare the two subtrees rooted at tpos and sibling
		int j;
		for (i = tpos, j = sibling; i < cand.size() && j < tpos; i++, j++) {
			if (cand[i] > cand[j])
				return check_canonical(cand, parent);
			else if (cand[i] < cand[j]) {
				return false;
			}
		}
	}
	return true;
}


bool lexsmaller(vector<int> &subtree, vector<int> &cand)
{
   int i,j;

   for (i=0, j=0; i < subtree.size() && j < cand.size();){

      if (subtree[i] > cand[j]){
         if (cand[j] != BranchIt) return false;
         else{
            while (cand[j] == BranchIt) j++;
            if (subtree[i] > cand[j]) return false;
            else if (subtree[i] < cand[j]) return true;
            else return false;
         }

      }
      else if (subtree[i] < cand[j]){
         if (subtree[i] != BranchIt) return true;
         else{
            while(subtree[i] == BranchIt) i++;
            if (subtree[i] > cand[j]) return false;
            else if (subtree[i] < cand[j]) return true;
            else return true;
         }
      }
      else{
         i++;
         j++;
      }
   }
   return false;
}

/*return a new node to previous prefix pattern. the new node with "val" will appends to the node in "pos"
 * we also can prune new node if one of new pattern's parent if not frequent.
 */
Element* test_node(int iter, SleuthPattern *eq, int val, int pos) {
	Element *eqn = NULL;

	//if noprune, return with a new Eqnode
	if (prune_type == noprune) {
		eqn = new Element(val, pos);
		return eqn;
	}


	assert(false); // the pruning logic below does not work for DFS, should be disabled
	//perform pruning

	//prune based on frequent subtree
	vector<int> cand;
	vector<int> subtree;

	int hval;
	int scope, scnt;

	//form the candidate preifx
	cand = eq->prefix();
	scnt = eq->get_scope(pos, scope); //what is the scope of node.pos

	while (scnt > scope) {
		cand.push_back(BranchIt);
		scnt--;
	}
	cand.push_back(val);

	bool res = true;

	//check subtrees
	int omita, omitb;
	res = true;
	//omit i-th item (omita) and associated BranchIt (omitb)
	int i, j, k;

	for (i = iter - 3; i >= 0; i--) {
		//find pos for i-th item
		for (j = 0, k = 0; j < cand.size(); j++) {
			if (cand[j] != BranchIt) {
				if (k == i) {
					omita = j;
					break;
				} else
					k++;
			}
		}

		//find pos for associated BranchIt
		scnt = 0;
		for (j++; j < cand.size() && scnt >= 0; j++) {
			if (cand[j] == BranchIt)
				scnt--;
			else
				scnt++;
		}
		if (scnt >= 0)
			omitb = cand.size();
		else
			omitb = j - 1;
		hval = 0;
		subtree.clear();
		bool rootless = false;
		scnt = 0;
		for (k = 0; k < cand.size() && !rootless; k++) {
			if (k != omita && k != omitb) {
				subtree.push_back(cand[k]);
				if (cand[k] != BranchIt) {
					hval += cand[k];
					scnt++;
				} else
					scnt--;
				if (scnt <= 0)
					rootless = true;

			}
		}

		//skip a rootless subtree
		if (!rootless && lexsmaller(subtree, cand)) {
			res = FK.find(iter - 1, subtree, hval);
			if (!res)
				return NULL; //subtree not found!
		}
	}

	if (res)
		eqn = new Element(val, pos);
	else
		eqn = NULL;

	return eqn;
}
/* After we generate a new child extension eqnode, update the projected database of the new eqnode.
 *  @l1 : the projected database for pattern A
 *  @l2 : the projected database for pattern B
 *  @ins : the new eqnode
 *  @st1, end1 : the begin and end position in l1
 *  @st2, end2 : the begin and end position in l2
*/
void check_ins(vector<SleuthProjTrans> *l1, vector<SleuthProjTrans> *l2, Element *ins, // inside
		int st1, int st2, int en1, int en2, bool f2, vector<SleuthProjTrans>* output_tlist) {
	SleuthProjTrans *n1, *n2;
	ival_type cmpval;

	bool found_flg = false; // child extension found?
	bool induced = false; // induced projected instance found for a join output?
	bool induced_flg = false; // induced projected instance found for the current transaction?
	int pos1 = st1; //temporary position holder for n1 ival
	bool next2; //should we go to next n2 ival? // once child find a parent enty, other entries can be skipped
	int depth; // if depth == 1, (child, parent) is an induced pattern

	while (st2 < en2) {
		n1 = &(*l1)[pos1];
		n2 = &(*l2)[st2];
		next2 = true; //by default we go to next n2 ival
		cmpval = ival::compare(n1->itscope, n2->itscope);

		switch (cmpval) {
		case sup:

			//n2 was under some n1, then such a extension is valid
			depth = n2->depth - n1->depth;
			if (depth >= 1 && n1->path_equal(*n2)) {
				if (depth == 1 && n1->induced)
					induced = true;
				else
					induced = false;
				if (!f2)
					depth = n2->depth;
				//construct new projected transaction and push it into the projected database of new child
				if (en1 - st1 > 1 || use_fullpath) {
				// project parent path into new projected transaction
					if(output_tlist)
						output_tlist->emplace_back(n2->tid, n1->parscope, n1->itscope.lb,
									n2->itscope, depth, induced);
					else
						ins->add_list(n2->tid, n1->parscope, n1->itscope.lb,
									n2->itscope, depth, induced);
				}
				else {
					if(output_tlist)
						output_tlist->emplace_back(n2->tid, n2->itscope, depth, induced);
					else
						ins->add_list(n2->tid, n2->itscope, depth, induced);
				}
				if (!count_unique)
					ins->sup++;
				found_flg = true;
				if (induced)
					induced_flg = true;
			}

			next2 = false;
			break;

		case before:
			//check next n1 ival for same n2 ival
			next2 = false;

			break;

		}

		if (next2 || pos1 + 1 == en1) { //go to next n2 ival
			pos1 = st1;
			st2++;
		} else
			pos1++; //go to next n1 ival
	}

	if (found_flg && count_unique)
		ins->sup++;
	if (induced_flg)
		ins->isup++;
}

/* After we generate a new cousin extension eqnode, update the projected database of the new eqnode
 *  @l1 : the projected database for pattern A
 *  @l2 : the projected database for pattern B
 *  @ins : the new eqnode
 *  @st1, end1 : the begin and end position in l1
 *  @st2, end2 : the begin and edn position in l2
*/
void check_outs(vector<SleuthProjTrans> *l1, vector<SleuthProjTrans> *l2, Element *outs,
		int st1, int st2, int en1, int en2, vector<SleuthProjTrans>* output_tlist) {
	SleuthProjTrans *n1, *n2;
	ival_type cmpval;

	bool found_flg = false;
	bool induced, induced_flg = false;
	bool next2;
	int pos1 = st1;

	while (st2 < en2) {
		n1 = &(*l1)[pos1];
		n2 = &(*l2)[st2];

		cmpval = ival::compare(n1->itscope, n2->itscope);

		switch (cmpval) {
		case sup:
			break;
		//if they have no intersect, then valid
		case before:
		case after:
			if (mine_ordered && cmpval == after)
				break;
			//n1 is before n2. Check if n1.par is subset of or equal to n2.par
			if (n1->path_equal(*n2)) {
				if (n1->induced && n2->induced)
					induced = true;
				else
					induced = false;
				//construct new projected transaction and push it into the projected database of new child
				if (en1 - st1 > 1 || use_fullpath) {
					// project parent path into new projected transaction
					if(output_tlist)
						output_tlist->emplace_back(n2->tid, n1->parscope, n1->itscope.lb,
									n2->itscope, n2->depth, induced);
					else
						outs->add_list(n2->tid, n1->parscope, n1->itscope.lb,
									n2->itscope, n2->depth, induced);
				}
				else {
					if(output_tlist)
						output_tlist->emplace_back(n2->tid, n2->itscope, n2->depth, induced);
					else
						outs->add_list(n2->tid, n2->itscope, n2->depth, induced);
				}
				if (!count_unique)
					outs->sup++;
				found_flg = true;
				if (induced)
					induced_flg = true;
			}

			break;
		case sub:
			break;

		}

		if (pos1 + 1 == en1) { //go to next n2 ival
			pos1 = st1;
			st2++;
		} else
			pos1++; //go to next n1 ival
	}

	if (found_flg && count_unique)
		outs->sup++;
	if (induced_flg)
		outs->isup++;
}

//do "scope list join" to get "inside" node and "outside" node's projected database, which is mentioned in zaki's sleuth work.
//l1 and l2 are two project database to join
//f2 means this function is called by getF2()
// the new generate projected database after join will store in ins->tlist and outs->tlist
void get_intersect(vector<SleuthProjTrans> *l1, vector<SleuthProjTrans> *l2, Element *ins,
		Element *outs, bool f2 = false) { //f2 is true only when we compute F2, set to false for Fk
	SleuthProjTrans *n1, *n2; //in Sleuth paper Fig 3 Vertical Format, n1 points to a row of a rectangle
	vector<TransBlock> tbs; // store all projected transaction ready to join
	// join the projected instances of the same transaction
	//i1 and i2 is the position where tid match begins, e1 and e2 is the position where tid match ends. The join operator will do in [i1, e1] and [i2, e2]
	int i1 = 0, i2 = 0;
	//
	int e1, e2;

	while (i1 < l1->size() && i2 < l2->size()) { // join
		n1 = &(*l1)[i1];
		n2 = &(*l2)[i2];

		//look for matching tids
		if (n1->tid < n2->tid)
			i1++;
		else if (n1->tid > n2->tid)
			i2++;
		else {
			//cids match
			e1 = i1;
			e2 = i2;

			//check the cid end positions in it1 and it2
			while (e1 < l1->size() && (*l1)[e1].tid == n1->tid)
				e1++;
			while (e2 < l2->size() && (*l2)[e2].tid == n2->tid)
				e2++;

			//generate projecetd transaction if candidate found
			if (ins) // child extension
				//we use + since the join is merge-like
				if((l1->size() + l2->size()) <= tauDB_omp)
					check_ins(l1, l2, ins, i1, i2, e1, e2, f2, 0); // in Sleuth paper Fig 3 Vertical Format, each range [i1, e1) denotes the rows for a transaction with some tid
				else tbs.emplace_back(i1, i2, e1, e2, true, f2);

			if (outs) // cousin extension
				//we use + since the join is merge-like
				if((l1->size() + l2->size()) <= tauDB_omp)
					check_outs(l1, l2, outs, i1, i2, e1, e2, 0); // in Sleuth paper Fig 3 Vertical Format, each range [i1, e1) denotes the rows for a transaction with some tid
				else tbs.emplace_back(i1, i2, e1, e2, false, f2);

			//restore index to end of cids
			i1 = e1;
			i2 = e2;
		}
	}
	//we use + since the join is merge-like
	if((l1->size() + l2->size()) > tauDB_omp){
		//do parallel join
		vector<vector<SleuthProjTrans> > ins_tlistOfThread(THREADS);
		vector<vector<SleuthProjTrans> > outs_tlistOfThread(THREADS);
		#pragma omp parallel for schedule(dynamic, CHUNK) num_threads(THREADS)
		for(int i = 0; i < tbs.size(); i++){
			TransBlock& cur = tbs[i];
			int thread_id = omp_get_thread_num();
			if(tbs[i].ins_flag) {
				check_ins(l1, l2, ins, cur.st1, cur.st2, cur.end1, cur.end2, cur.f2, &ins_tlistOfThread[thread_id]);
			}
			else {
				check_outs(l1, l2, outs, cur.st1, cur.st2, cur.end1, cur.end2, &outs_tlistOfThread[thread_id]);
			}
		}

		if(ins){
			for(int i = 0; i < THREADS; i++){
				vector<SleuthProjTrans>& tlist_i = ins_tlistOfThread[i];
				if(ins->tlist.empty()) ins->tlist.swap(tlist_i);
				else{
					ins->tlist.insert(ins->tlist.end(), tlist_i.begin(), tlist_i.end());
				}
			}
			sort(ins->tlist.begin(), ins->tlist.end(), projTrans_cmp);
		}
		if(outs) {
			for(int i = 0; i < THREADS; i++){
				vector<SleuthProjTrans>& tlist_i = outs_tlistOfThread[i];
				if(outs->tlist.empty()) outs->tlist.swap(tlist_i);
				else{
					outs->tlist.insert(outs->tlist.end(), tlist_i.begin(), tlist_i.end());
				}
			}
			sort(outs->tlist.begin(), outs->tlist.end(), projTrans_cmp);
		}
	}
}
#endif
