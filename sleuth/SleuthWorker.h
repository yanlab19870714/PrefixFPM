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

#ifndef __SLEUTHWORKER_H
#define __SLEUTHWORKER_H

#include <iostream>
#include <fstream>
#include <vector>

#include "SleuthTask.h"

#define ITSZ sizeof(int)
#define buf_sz 2048
#define TRANSOFF 3

class SleuthWorker: public Worker<SleuthTask> {
private:
	int buf_size;
	int * buf;
	int cur_blk_size;
	int cur_buf_pos;
	int endpos;
	char readall;

public:

	//vars related to vertical format
	vector<vector<SleuthProjTrans> *> Idlists;

	//function definitions
	SleuthWorker(const char *infile, const char *outfolder = "outputs"): Worker(infile, outfolder){
		buf_size = buf_sz;
		buf = new int[buf_sz];
		cur_buf_pos = 0;
		cur_blk_size = 0;
		readall = 0;
		fd.seekg(0, ios::end);
		endpos = fd.tellg();
		fd.seekg(0, ios::beg);
	}

	~SleuthWorker() {
		for(int i = 0; i<TransDB().size(); i++)
			delete TransDB()[i];
		for(int i = 0; i<Idlists.size(); i++)
			delete Idlists[i];
		delete[] buf;
		delete[] FreqIdx;
		delete[] FreqMap;
	}

	//functions for horizontal format
	void get_next_trans_ext() {
		// Need to get more items from file
		int res = cur_blk_size - cur_buf_pos;
		if (res > 0) {
			// First copy partial transaction to beginning of buffer
			for (int i = 0; i < res; i++)
				buf[i] = buf[cur_buf_pos + i];
			cur_blk_size = res;
		} else {
			// No partial transaction in buffer
			cur_blk_size = 0;
		}

		fd.read((char *) (buf + cur_blk_size),
				((buf_size - cur_blk_size) * ITSZ));

		res = fd.gcount();
		if (res < 0) {
			cerr << "in get_next_trans_ext" << res << endl;
		}

		if (res < (buf_size - cur_blk_size)) {
			fd.clear();
			fd.seekg(0, ios::end);
		}

		cur_blk_size += res / ITSZ;
		cur_buf_pos = 0;
	}

	void get_first_blk() {
		readall = 0;

		fd.clear();
		fd.seekg(0, ios::beg);

		if (binary_input) {
			fd.read((char *) buf, (buf_size * ITSZ));
			cur_blk_size = fd.gcount() / ITSZ;
			if (cur_blk_size < 0) {
				cerr << "problem in get_first_blk" << cur_blk_size << endl;
			}
			if (cur_blk_size < buf_size) {
				fd.clear();
				fd.seekg(0, ios::end);
			}
		}
		cur_buf_pos = 0;
	}
	virtual int getNextTrans(vector<TransT>& transDB) {
		static char first = 1;
		int Cid;
		int Tid;
		int TransSz;
		int* TransAry;

		if (first) {
			first = 0;
			get_first_blk();
		}

		if (binary_input) {
			if (cur_buf_pos + TRANSOFF >= cur_blk_size
					|| cur_buf_pos + buf[cur_buf_pos + TRANSOFF - 1] + TRANSOFF
							> cur_blk_size) {
				fd.seekg(0, ios::cur);
				if (((int) fd.tellg()) == endpos)
					readall = 1;
				if (!readall) {
					// Need to get more items from file
					get_next_trans_ext();
				}
			}

			if (eof()) {
				first = 1;
				return 0;
			}

			if (!readall) {
				Cid = buf[cur_buf_pos];
				Tid = buf[cur_buf_pos + TRANSOFF - 2];
				TransSz = buf[cur_buf_pos + TRANSOFF - 1];
				TransAry = buf + cur_buf_pos + TRANSOFF;
				TransDB().push_back(new SleuthTrans(TransAry, TransSz, Tid, Cid));
				cur_buf_pos += TransSz + TRANSOFF;
			}
			return 1;
		}
		//not binary
		else {
			if ((int) fd.tellg() == endpos - 1) {
				readall = 1;
				first = 1;
				return 0;
			} else {
				int i;
				fd >> Cid;
				fd >> Tid;
				fd >> TransSz;
				for (i = 0; i < TransSz; ++i) {
					fd >> buf[i];
				}
				TransAry = buf;
				TransDB().push_back(new SleuthTrans(TransAry, TransSz, Tid, Cid));
				cur_buf_pos = 0;

				return 1;
			}
		}
	}

	//delete infrequent elements in transDB
	void get_valid_trans(SleuthTrans &t) {
		int i, j;
		int newTransSz;

		vector<int>& TransAry = t.TransAry;
		for (i = 0; i < TransAry.size(); i++) {
			if (TransAry[i] != InvalidIt) {
				if (TransAry[i] != BranchIt) {
					if (FreqMap[TransAry[i]] == -1) {
						//set item to invalid and the next -1 to invalid
						TransAry[i] = InvalidIt;
						int cnt = 0;

						for (j = i + 1; j < TransAry.size() && cnt >= 0; j++) {
							if (TransAry[j] != InvalidIt) {
								if (TransAry[j] == BranchIt)
									cnt--;
								else
									cnt++;
							}
						}
						if (!mine_induced)
							TransAry[--j] = InvalidIt;
					}
				}
			}
		}


		//copy valid items to PvtTransAry
		if (mine_induced) {
			//just copy all items (invalid, branch, regular) since we do not
			//want to count embedded occurrences as induced.
			for (i = 0, newTransSz = 0; i < TransAry.size(); i++) {
				if (TransAry[i] == BranchIt || TransAry[i] == InvalidIt)
					TransAry[newTransSz] = TransAry[i];
				else
					TransAry[newTransSz] = FreqMap[TransAry[i]];
				newTransSz++;
			}
		} else {
			//eliminate invalid items and compress the tree
			for (i = 0, newTransSz = 0; i < TransAry.size(); i++) {
				if (TransAry[i] != InvalidIt) {
					if (TransAry[i] == BranchIt)
						TransAry[newTransSz] = TransAry[i];
					else
						TransAry[newTransSz] = FreqMap[TransAry[i]];
					newTransSz++;
				}
			}
		}
		TransAry.resize(newTransSz);
	}

	int eof() {
		return (readall == 1);
	}

	//functions to get vertical format in a transaction
	void make_vertical(SleuthTrans& t) {

		int i, j; //track the position in trans, counting BranchIt
		int pi, pj; //track the position in trans, not counting BranchIt
		int scope;
		int depth = 0;
		//convert current transaction into vertical format

		for (i = 0, pi = 0; i < t.TransAry.size(); i++) {

			if (t.TransAry[i] == BranchIt) {
				depth--;
				continue;
			} else if (t.TransAry[i] == InvalidIt) {
				depth++;
				pi++;
				continue;
			}

			scope = 0;
			for (j = i + 1, pj = pi; scope >= 0 && j < t.TransAry.size(); j++) {
				if (t.TransAry[j] == BranchIt)
					scope--;
				else {
					scope++;
					pj++;
				}
			}
			Idlists[t.TransAry[i]]->emplace_back(t.tid, pi, pj, depth, true);
			pi++;
			depth++;
		}
	}
	void print_vertical() {
		int i;
		for (i = 0; i < NumF1; i++) {
			fouts[0] << "item " << FreqIdx[i] << endl;
			fouts[0] << *Idlists[i];
		}
	}
	void alloc_idlists() {
		Idlists.resize(NumF1);
		for (int i = 0; i < NumF1; i++) {
			Idlists[i] = new vector<SleuthProjTrans>;// todo check has deleted???
		}
	}

	void get_F1() {
		int i, j, it;
		const int arysz = 10;

		vector<int> itcnt(arysz, 0); //count item frequency, itcnt[val] = count
		vector<int> flgs(arysz, -1);


		DBASE_MAXITEM = 0; // the max value in tree/

		//traverse all tree to get the item frequency
		for (int m = 0; m < TransDB().size(); m++) {
			for (i = 0; i < TransDB()[m]->TransAry.size(); i++) {
				it = TransDB()[m]->TransAry[i];
				if (it == BranchIt)
					continue;

				if (it >= DBASE_MAXITEM) {
					itcnt.resize(it + 1, 0);
					flgs.resize(it + 1, -1);
					DBASE_MAXITEM = it + 1;
				}

				if (count_unique) {
					if (flgs[it] == TransDB()[m]->tid)
						continue;
					else
						flgs[it] = TransDB()[m]->tid;
				}
				itcnt[it]++;
			}

			if (MaxTransSz < TransDB()[m]->TransAry.size())
				MaxTransSz = TransDB()[m]->TransAry.size();
		}

		//set the value of MINSUPPORT
		if (minsup == -1)
			minsup = (int) (MINSUP_PER * TransDB().size() + 0.5);

		if (minsup < 1)
			minsup = 1;
		fouts[0] << "DBASE_NUM_TRANS : " << TransDB().size() << endl;
		fouts[0] << "DBASE_MAXITEM : " << DBASE_MAXITEM << endl;
		fouts[0] << "MINSUPPORT : " << minsup << " (" << MINSUP_PER << ")"
				<< endl;

		NumF1 = 0; //total count of frequent items
		for (i = 0; i < DBASE_MAXITEM; i++)
			if (itcnt[i] >= minsup)
				NumF1++;

		//do ordering for all items (both frequent and infrequent) with given sort_type
		int *it_order = new int[DBASE_MAXITEM];
		for (i = 0; i < DBASE_MAXITEM; i++)
			it_order[i] = i;
		if (sort_type != nosort) {
			ITCNT = &itcnt;
			sort(&it_order[0], &it_order[DBASE_MAXITEM], F1cmp);
		}

		//construct forward and reverse mapping from index to frequent items
		FreqIdx = new int[NumF1];
		FreqMap = new int[DBASE_MAXITEM];
		for (i = 0, j = 0; i < DBASE_MAXITEM; i++) {
			if (itcnt[it_order[i]] >= minsup) {
				if (output)
					fouts[0] << it_order[i] << " - " << itcnt[it_order[i]] << endl;
				FreqIdx[j] = it_order[i];
				FreqMap[it_order[i]] = j;
				j++;
			} else
				FreqMap[it_order[i]] = -1; //-1 means infrequent items
		}

		if (sort_type != nosort) {
			ITCNT = NULL;
		}
		delete[] it_order;
	}

	void get_F2(stack<SleuthTask*>& task_queue) {
		int i, j;
		int it1, it2;
		int scnt;

		double te;


		//itcnt2 is a matrix of pairs p, p.first is count, p.second is flag
		int **itcnt2 = new int*[NumF1];
		int **flgs = new int*[NumF1];
		for (i = 0; i < NumF1; i++) {
			itcnt2[i] = new int[NumF1];
			flgs[i] = new int[NumF1];
			for (j = 0; j < NumF1; j++) {
				itcnt2[i][j] = 0;
				flgs[i][j] = -1;
			}
		}

		// first we allocate space for vertical databases for each node, then fill them by make_vertical.
		// Then we construct two-node trees by nested loop, and get their frequency
		alloc_idlists(); // allocate space for frequent items
		for (int m = 0; m < TransDB().size(); m++) {
			//delete infrequent nodes and edges
			get_valid_trans(*TransDB()[m]);
			SleuthTrans* validT =  TransDB()[m];
			make_vertical(*validT);
			//count a pair only once per tid
			for (i = 0; i < validT->TransAry.size(); i++) {
				it1 = validT->TransAry[i];
				if (it1 == InvalidIt)
					continue;
				if (it1 != BranchIt) {
					scnt = 0;
					for (j = i + 1; scnt >= 0 && j < validT->TransAry.size(); j++) {
						it2 = validT->TransAry[j];
						if (it2 != BranchIt) {
							scnt++;
							if (it2 == InvalidIt)
								continue;
							if (count_unique) {
								if (flgs[it1][it2] == validT->tid)
									continue;
								else
									flgs[it1][it2] = validT->tid;
							}
							itcnt2[it1][it2]++;
						} else
							scnt--;
					}
				}
			}
		}

		int F2cnt = 0; //the count of frequent 2-items
		// count frequent patterns and generate eqclass
		for (i = 0; i < NumF1; i++) {
			SleuthTask* t = new SleuthTask;
			t->iter = 2;
			SleuthPattern& eq = t->pat;
			for (j = 0; j < NumF1; ++j) {
				if (itcnt2[i][j] >= minsup) {
					F2cnt++;

					if (eq.isEmpty()) {
						eq.prefix().push_back(i);
					}

					eq.add_node(j, 0);
				}
			}
			if(eq.isEmpty()) continue;
			form_f2_lists(t->pat); //generate projected database for current pattern
			task_queue.push(t);
		}

		for (i = 0; i < NumF1; i++) {
			delete[] itcnt2[i];
			delete[] flgs[i];
		}

		delete[] itcnt2;
		delete[] flgs;

		BranchIt = NumF1 + 100; //make branchit more than all other items

		return;
	}



	void form_f2_lists(SleuthPattern& eq) {
		vector<Element *>::iterator ni;
		vector<SleuthProjTrans> *l1, *l2;
		Element *ins = NULL, *outs = NULL;
		int pit, nit;
		pit = eq.prefix()[0];
		l1 = Idlists[pit];
		for(int i = 0; i<eq._nodelist.size(); i++){
			nit = eq._nodelist[i]->val;
			l2 = Idlists[nit];
			ins = eq._nodelist[i];
			get_intersect(l1, l2, ins, outs, true);

		}
	}

	virtual void setRoot(stack<SleuthTask*>& task_queue){
		get_F1();
		get_F2(task_queue);
	}

};


#endif //__DATABASE_H
