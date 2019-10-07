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

#include "prefixspan/prefixspan.h"
//#include "gspan/GspanWorker.h"
//#include "sleuth/SleuthWorker.h"

#include "system/timetrack.h"

char infile[300];

void parse_args(int argc, char **argv) {
		extern char *optarg;
		int c;

		if (argc < 2)
			cout << "usage:  -i <infile> -s <support> -n <thread_number>\n";
		else {
			while ((c = getopt(argc, argv, "i:s:n:c:T:t:b:B")) != -1) {
				switch (c) {
				case 'i': //input files
					sprintf(infile, "%s", optarg);
					break;
				case 's': //support value for L2
					minsup = atof(optarg);
					break;
				case 'n':
					THREADS = atoi(optarg);
					break;
				case 'B': // the input file is ASCII(or binary file)
					binary_input = true;
					break;
				case 'c': //how many elements per access in OMP parallel-for that processes PDB rows
					CHUNK = atoi(optarg);
					break;
				case 'T': // projDB size threshold, below which task is run in a single thread
					tauDB_omp = atof(optarg);
					break;
				case 't': // projDB size threshold, below which task is run in a single thread
					tauDB_singlethread = atof(optarg);
					break;
				case 'b': //how many tasks per access to the task queue
					batch_size = atoi(optarg);
					break;
				}
			}
		}
	}


int main(int argc, char **argv){
	parse_args(argc, argv);
	TimeTracker tt;
	PrefixWorker worker(infile);
	//GspanWorker worker(infile);
	//SleuthWorker worker(infile);
	worker.read();
	tt.start();
	worker.run();
	cout << tt.stop() << endl;
	return 0;
}


//./run -s 100 -i prefixspan/test.data -n 4
