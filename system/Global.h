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

#ifndef SYSTEM_GLOBAL_H_
#define SYSTEM_GLOBAL_H_

#include <fstream>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mutex>

using namespace std;

int THREADS = 4;// number of threads
int CHUNK = 1;// how many elements per access in OMP parallel-for that processes PDB rows
int batch_size = 1; // how many tasks per access to the task queue
int minsup = 1;
int tauDB_omp = 10000000; // projDB size threshold, below which task is run in a single thread
int tauDB_singlethread = 100; // projDB size threshold, below which task is run in a single thread
bool binary_input = false; // can only be true for Sleuth

#define WAIT_TIME_WHEN_IDLE 100

//global transaction DB (type to be defined)
void* transDB;
//global task queue (type to be defined)
void* task_queue;
mutex q_mtx;

//an array of ofstreams, one for each comper thread
string outFolder; // "outputs" by default
ofstream* fouts;
//fouts[0] may also be used by the main thread
//fouts[i] is used by computing thread i

//disk operations
void _mkdir(const char *dir) {//taken from: http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if(tmp[len - 1] == '/') tmp[len - 1] = '\0';
	for(p = tmp + 1; *p; p++)
		if(*p == '/') {
				*p = 0;
				mkdir(tmp, S_IRWXU);
				*p = '/';
		}
	mkdir(tmp, S_IRWXU);
}

#endif /* SYSTEM_GLOBAL_H_ */
