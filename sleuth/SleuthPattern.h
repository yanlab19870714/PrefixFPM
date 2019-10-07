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

#ifndef SLEUTHPATTERN_H_
#define SLEUTHPATTERN_H_
#include "SleuthTrans.h"
#include <algorithm>

template<class T>
struct delnode: public unary_function<T, void>{
   void operator() (T x){ delete x; }
};

//Pattern here contains a set of candidates, which have the same prefix
class SleuthPattern: public Pattern{
public:
   vector<int> _prefix; //eq-class's prefix pattern listed as recoded labels
   vector<Element *> _nodelist; // child pattern nodes to expand with
   int total_tlist;

   SleuthPattern(){
	   total_tlist = 0;
   }

   ~SleuthPattern(){
	   for_each(nlist().begin(), nlist().end(), delnode<Element *>());
   }

   bool isEmpty(){
	   return (_prefix.empty()==true);
   }

   vector<int> &prefix(){ return _prefix; }

   vector<Element *> &nlist(){ return _nodelist; }

   void add_node(int val, int pos, int sup=0)
   {
	   Element *eqn = new Element(val,pos,sup);
      _nodelist.push_back(eqn);
   }

   void add_node(Element *eqn)
   {
      _nodelist.push_back(eqn);
   }

   //return nth element in prefix
   int item(int n) {
      int i,k;
      for (i=0,k=0; i < _prefix.size(); i++){
         if (_prefix[i] != BranchIt){
            if (k == n) return _prefix[i];
            k++;
         }
      }
      return -1;
   }

   //returns the depth of last pattern node
   //"scope" also gets the depth of prefix pattern node at "pos"
   int get_scope(int pos, int &scope){
     int scnt=0;
     for (int i=0; i < _prefix.size(); i++){
       if (_prefix[i] == BranchIt) scnt--;
       else scnt++;
       if (i == pos) scope = scnt;
     }
     return scnt;
   }

   //set the current pattern's "_prefix" field using an item in the _nodelist
   void set_prefix(vector<int> &pref, const Element &node)
   {
      _prefix = pref;

      int scope, scnt;
      scnt = get_scope(node.pos, scope);

      while(scnt > scope){
        _prefix.push_back(BranchIt);
        scnt--;
      }
      _prefix.push_back(node.val);
    }

   void sort_nodes(){
      if (sort_type == nosort) return;
      sort(_nodelist.begin(), _nodelist.end(), Element::supcmp);
   }

   //print the prefix. After call print(), it is still needed to print the nodes in _nodelist
   void print(ostream& fout)
   {
      for (int i=0; i < _prefix.size(); i++){
         if (_prefix[i] == BranchIt) fout << LOCAL_BRANCHIT << " ";
         else fout << FreqIdx[_prefix[i]] << " ";
      }
   }

};


#endif /* PATTERN_H_ */
