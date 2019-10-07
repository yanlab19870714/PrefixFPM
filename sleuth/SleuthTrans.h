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

#ifndef SLEUTHTRANS_H_
#define SLEUTHTRANS_H_

#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <cstring>

#include "Utils.h"

//comparison result of 2 intervals
enum ival_type {
	equals, sup, sub, before, after, overlap
};

//interval class
class ival {
public:
	int lb; //lower bound of scope range
	int ub; //upper bound of scope range

	ival(int l = 0, int u = 0) :
			lb(l), ub(u) {
	}

	bool operator ==(ival &iv) {
		return (iv.lb == lb && iv.ub == ub);
	}

	static ival_type compare(ival &n1, ival &n2) {
		ival_type retval;
		if (n1.lb == n2.lb && n1.ub == n2.ub)
			retval = equals; //n1 equals n2
		else if (n1.lb < n2.lb && n1.ub >= n2.ub)
			retval = sup; //n1 contains n2
		else if (n1.lb > n2.lb && n1.ub <= n2.ub)
			retval = sub; //n2 contains n1
		else if (n1.ub < n2.lb)
			retval = before; //n1 comes before n2
		else if (n2.ub < n1.lb)
			retval = after; //n1 comes after n2
		else {
			retval = overlap;
		}
		return retval;
	}
};

// in paper, fig 3 right part, each item in a rectangle
class SleuthProjTrans: public ProjTrans {
public:
	ival itscope; //node's scope
	vector<int> parscope; // projected node IDs (in a transaction) of the prefix pattern
	int depth; //node's depth in the projected subtree with given tid (number of nodes on the tree path)
	bool induced;

	SleuthProjTrans(int t = 0, int l = 0, int u = 0, int d = 0, bool i = false) :
		ProjTrans(t), itscope(l, u), depth(d), induced(i) {
	}

	SleuthProjTrans(int t, ival &it, int d, bool i = false): ProjTrans(t) {
		itscope = it;
		depth = d;
		induced = i;
	}

	SleuthProjTrans(int t, vector<int> &pscope, int lit, ival &ivl, int d, bool i =
			false): ProjTrans(t){
		itscope = ivl;
		parscope = pscope;
		parscope.push_back(lit);
		depth = d;
		induced = i;
	}

	//check whether has the same projected prefix-path as "idn" in a transaction
	bool path_equal(SleuthProjTrans &idn) {
		int i;
		for (i = 0; i < parscope.size(); i++) {
			if (parscope[i] != idn.parscope[i])
				return false;
		}
		return true;
	}

	void project(SleuthProjTrans* newProj){
		newProj->parscope = parscope;
		newProj->parscope.push_back(itscope.lb);
	}
};

bool projTrans_cmp(const SleuthProjTrans& a, const SleuthProjTrans& b){
	if(a.tid < b.tid) return true;
	if(a.tid == b.tid && a.itscope.lb < b.itscope.lb) return true;
	if(a.tid == b.tid && a.itscope.lb == b.itscope.lb && a.itscope.ub <b.itscope.ub) return true;
	return false;
}
ostream & operator<<(ostream& fout, vector<int> &vec) {
	for (int i = 0; i < vec.size(); i++)
		fout << vec[i] << " ";
	return fout;
}

ostream & operator<<(ostream& ostr, ival& idn){
   ostr << "[" << idn.lb << ", " << idn.ub << "]";
   return ostr;
}

ostream & operator<<(ostream& ostr, SleuthProjTrans& idn) {
	ostr << idn.tid << " " << idn.itscope << " -- " << idn.parscope << " , "
			<< idn.depth << " " << idn.induced;
	return ostr;
}

ostream & operator<<(ostream& fout, vector<SleuthProjTrans>& idl) {
	for (int j = 0; j < idl.size(); j++) {
		fout << "\t" << idl[j] << endl;
	}
	return fout;
}

class Eqnode {
public:
	int val; //recoded label of the last node in pattern
	int pos; //parent node's ID in pattern
	atomic<int> sup; //support
	atomic<int> isup; //induced support
	vector<SleuthProjTrans> tlist; //scope list for this item
	// in paper, fig 3 right part, each rectangle

	Eqnode(int v, int p, int s = 0, int i = 0) :
			val(v), pos(p), sup(s), isup(i) {
	}

	static bool supcmp(Eqnode *n1, Eqnode *n2) {//todo think why increasing order
		bool res = false;
		if ((n1)->sup < (n2)->sup)
			res = true;

		return res;
	}

	void add_list(int t, ival &it, int d, bool i = false) {
		tlist.emplace_back(t, it, d, i);
	}

	void add_list(int t, vector<int> &pscope, int lit, ival &ivl, int d,
			bool i = false) {
		tlist.emplace_back(t, pscope, lit, ivl, d, i);
	}

	friend ostream & operator<<(ostream& ostr, Eqnode& eqn) {
		int *fidx = FreqIdx;
		if (mine_induced)
			ostr << " / " << eqn.isup << endl;
		else
			ostr << " / " << eqn.sup << endl;
		if (output_idlist)
			ostr << eqn.tlist;
		return ostr;
	}
};

typedef Eqnode Element;

class SleuthTrans:public Trans {
public:
	vector<int> TransAry;
	int Tid;

	SleuthTrans(int* _TransAry, int _TransSz, int _Tid, int _Cid):Trans(_Cid) {
		for(int i = 0; i< _TransSz; i++)
			TransAry.push_back(_TransAry[i]);
		Tid = _Tid;
	}

	SleuthTrans(const SleuthTrans& t):Trans(t.tid) {
		TransAry = t.TransAry;
		Tid = t.Tid;
	}
};
#endif /* TRANSACTION_H_ */
