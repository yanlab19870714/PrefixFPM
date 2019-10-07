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

#ifndef __sleuth_hasht_h
#define __sleuth_hasht_h

#include <map> 
#include <ext/hash_map>
#include <list>
#include <vector>
#include <functional>

#include "SleuthPattern.h"

#define HASHNS __gnu_cxx
using namespace std;

#define FHTSIZE 100 
//for pruning candidate subtrees
typedef HASHNS::hash_multimap<int, vector<int> *, HASHNS::hash<int>, equal_to<int> > cHTable;
typedef pair < cHTable::iterator, cHTable::iterator> cHTFind;
typedef cHTable::value_type cHTPair;
bool eqcmp(vector<int> *v1, vector<int> *v2)
{
   if (v1->size() != v2->size()) return false;
   for (int i=0; i < v1->size(); i++){
      if ((*v1)[i] != (*v2)[i]) return false;
   }
   return true;
}
class FreqHT{
   vector<cHTable *> chtable;
public:
   FreqHT(int sz = FHTSIZE): chtable(sz, ((cHTable *) NULL)){}
   ~FreqHT(){ clearall(); }

   void clearall(){
      for (int i=0; i < chtable.size(); i++){
         if (chtable[i]){
            cHTable::iterator hi = chtable[i]->begin();
            for (; hi != chtable[i]->end(); hi++){
               delete (*hi).second;
            }
            chtable[i]->clear();
         }
      }
   }
   
   void add(int iter, SleuthPattern *eq){
      vector<int> *iset;
      int phval = 0;
      int i;
      for (i=0; i < eq->prefix().size(); i++)
         if (eq->prefix()[i] != BranchIt) phval += eq->prefix()[i];


      int hval = 0;
      int scope, scnt;
      vector<Element *>::iterator ni = eq->nlist().begin();
      for (; ni != eq->nlist().end(); ni++){
         iset = new vector<int>(eq->prefix());
         scnt = eq->get_scope((*ni)->pos, scope); //what is the scope of node.pos
         while(scnt > scope){
            iset->push_back(BranchIt);
            scnt--;
         }
         iset->push_back((*ni)->val);
         hval = phval + (*ni)->val;
         if (chtable[iter] == NULL) chtable[iter] = new cHTable(FHTSIZE);
         int hres = chtable[iter]->hash_funct()(hval);
         chtable[iter]->insert(cHTPair(hres, iset));
         //cout << "ADD " << hres << " xx " << *iset << endl;
      }
   }


   bool find(int iter, vector<int> &cand, int hval)
   {
      if (chtable[iter] == NULL) return false;

      int hres = chtable[iter]->hash_funct()(hval);
      cHTFind p = chtable[iter]->equal_range(hres);

      //cout << "FIND " << hres << " xx  " << cand << endl;
      cHTable::iterator hi = p.first;
      for (; hi!=p.second; hi++){
         if (eqcmp(&cand, (*hi).second)) return true;
      }
      return false;
   }
};



#endif
