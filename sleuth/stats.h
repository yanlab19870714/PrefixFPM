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

#ifndef __STATS_H
#define __STATS_H


#include<iostream>
#include<vector>
using namespace std;

class iterstat{
public:
   iterstat(int nc=0, int nl=0, double tt=0.0,double aa=0.0):
      numcand(nc),numlarge(nl),time(tt),avgtid(aa){}
   int numcand;
   int numlarge;
   double time;
   double avgtid;
};
 
class Stats: public vector<iterstat>{
public:
   static double sumtime; // current time cost
   static int sumcand;
   static int sumlarge;
   static double tottime;

   //Stats();

   void add(iterstat &is){
      push_back(is);
      sumtime += is.time;
      sumcand += is.numcand;
      sumlarge += is.numlarge;
   }
   void add(int cand, int freq, double time, double avgtid=0.0){
     iterstat *is = new iterstat(cand, freq, time, avgtid);
     push_back(*is);
     sumtime += is->time;
     sumcand += is->numcand;
     sumlarge += is->numlarge;
   }
   void incrcand(int pos, int ncand=0)
   {
      if (pos >= size()) resize(pos+1);
      (*this)[pos].numcand += ncand;
      sumcand += ncand;
   }

   void incrlarge(int pos, int nlarge=0)
   {
      if (pos >= size()) resize(pos+1);
      (*this)[pos].numlarge += nlarge;
      sumlarge += nlarge;
   }

   void incrtime(int pos, double ntime)
   {
      if (pos >= size()) resize(pos+1);
      (*this)[pos].time += ntime;
      sumtime += ntime;
   }
   friend ostream& operator << (ostream& fout, Stats& stats){
	   //fout << "SIZE " << stats.size() << endl;
	   for (int i=0; i<stats.size(); i++){
	     fout << "[ " << i+1 << " " << stats[i].numcand << " "
	 	 << stats[i].numlarge << " " << stats[i].time << " "
	 	 << stats[i].avgtid << " ] ";
	   }
	   fout << "[ SUM " << stats.sumcand << " " << stats.sumlarge << " "
	        << stats.sumtime << " ] ";
	   fout << stats.tottime;
	   return fout;
	 }
};
//static initializations
double Stats::tottime = 0;
double Stats::sumtime = 0;
int Stats::sumcand = 0;
int Stats::sumlarge = 0;
#endif
