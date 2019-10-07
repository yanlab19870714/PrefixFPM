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

#include "stats.h"
#include "Trans.h"
#include "Pattern.h"
#include "timetrack.h"
#include "Task.h"
#include "Worker.h"
#include "Global.h"

#include "omp.h"

using namespace std;

#define LOCAL_BRANCHIT -1 //backtracking symbol
static int NumF1;   //number of freq items
//recode node labels into 0, 1, 2, ...
static int *FreqMap; //FreqMap[label] = recoded_Label_ID, can be replace with map
static int *FreqIdx; //FreqIdx[recoded_Label_ID] = label


//enums
enum sort_vals {
	nosort, incr, decr
};
enum prune_vals {
	noprune, prune
};

//global vars
long BranchIt = -1; //-1 indicates a branch in the tree
long InvalidIt = -3; // used for removing infrequent items

atomic<int> ID(0); //give ID to the output patterns

double MINSUP_PER; //support in percentage
int DBASE_MAXITEM; //maximum node label, size of FreqMap
vector<int> *ITCNT = NULL; //count array where itcnt[label] = number of transactions containing the label, used for sorting F1

//default flags
bool output = true; //whether to print freq subtrees
bool output_idlist = false; //print the transID-list for each pattern
bool count_unique = true; //count support only once per transaction
bool use_fullpath = false; //for a matched projTrans, whether to keep track of all matched nodes, or just the last one
bool mine_induced = false; // whether to consider induced patterns (less pruning allowed); only when it is true, "check_isup" makes sense
bool mine_ordered = false; // whether tree-pattern is ordered (consider children as sequence rather than itemset)
static int MaxTransSz = 0; // length of the longest transaction
bool check_isup= false ; // whether to compute induced support

sort_vals sort_type = nosort; // whether to sort labels by frequency (listed in FreqIdx)
prune_vals prune_type = noprune;//todo have problem when prune**

bool F1cmp(int x, int y) {
	bool res = false;
	if ((*ITCNT)[x] < (*ITCNT)[y])
		res = true;

	if (sort_type == incr)
		return res;
	else
		return !res;
}
