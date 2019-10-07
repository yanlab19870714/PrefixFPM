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

#ifndef WORKER_H_
#define WORKER_H_

#include <stack>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <thread>

#include "Global.h"

template<class TaskT>
class Worker {
public:
	typedef typename TaskT::TransType TransT; // can be a pointer, if so, delete transDB elements in destructor

	ifstream fd; // input file stream

	vector<TransT>& TransDB(){ // get the global transaction database
		return *(vector<TransT>*)transDB;
	}

	stack<TaskT*>& tqueue(){ // get the global transaction database
		return *(stack<TaskT*> *)task_queue;
	}

	//end tag managed by main thread
	atomic<bool> global_end_label; //end tag, to be set by main thread
	size_t global_num_idle; // how many tasks are idle, protected by mtx_go

	Worker(const char *infile, const char *outfolder = "outputs"){
		//create global transaction database
		transDB = new vector<TransT>;
		//create global transaction database
		task_queue = new stack<TaskT*>;

		//set global end label to be false. when task queue is empty and no thread is runing, set it to be true
		global_end_label = false;
		global_num_idle = 0;

		//set the input stream
		if (binary_input) { // only supported by Sleuth, not prefixspan or gspan
			fd.open(infile, ios::in | ios::binary);
			if (!fd) {
				cerr << "cannot open input file " << infile << endl;
				exit(1);
			}
		} else {
			fd.open(infile, ios::in);
			if (!fd) {
				cerr << "cannot open input file " << infile << endl;
				exit(1);
			}
		}
		//set output folder and output streams
		outFolder = outfolder;
		_mkdir(outfolder);
		fouts = new ofstream[THREADS];
		for(int i=0; i<THREADS; i++)
			fouts[i].open(outFolder+"/"+to_string(i));
	};

	virtual void setRoot(stack<TaskT*>& task_queue) = 0; // put root tasks into task_queue
	// preprocessing code like finding frequent items should also go here

	virtual int getNextTrans(vector<TransT>& transDB) = 0; //to read a transaction from input file stream, and put it into transDB
	// return 0 when reaching end of file

	void read() { //read all lines
		vector<TransT>& db = TransDB();
		while (getNextTrans(db));
	}

	// ============== main logic begin =================
	//set of variables for conditioning on whether to run a task-processing thread
	mutex mtx_go;
	condition_variable cv_go;
	bool ready_go = true; //protected by mtx_go

	bool get_and_process_tasks(ofstream& fout, int batch_size = 1){
		queue<TaskT *> collector;
		q_mtx.lock();
		while(!tqueue().empty() && batch_size>0){
			TaskT* task = (TaskT*)tqueue().top();
			tqueue().pop();
			collector.push(task);
			batch_size--;
		}
		q_mtx.unlock();

		if(collector.empty()) return false;

		//process tasks in "collector"
		while(!collector.empty()){
			TaskT* task = collector.front();
			collector.pop();
			task->run(fout);
			delete task;
		}

		return true;
	}

	void thread_run(ofstream& fout){
	    while(global_end_label == false) //otherwise, thread terminates
	    {
	        bool busy = get_and_process_tasks(fout);
	        if(!busy) //if task_queue is empty
	        {
	            unique_lock<mutex> lck(mtx_go);
	            ready_go = false;
	            global_num_idle++;
	            while(!ready_go)
	            {
	                cv_go.wait(lck);
	            }
	            global_num_idle--;
	        }
	    }
	}

	void parallel_run(){
		//------------------------ create computing threads
	    vector<thread> threads;
	    for(int i=0; i<THREADS; i++)
	    {
	        threads.push_back(thread(&Worker<TaskT>::thread_run, this, ref(fouts[i])));
	    }
	    //------------------------
	    while(global_end_label == false)
	    {
	        usleep(WAIT_TIME_WHEN_IDLE); //avoid busy-checking
	        //------
	        q_mtx.lock();
	        if(!tqueue().empty())
	        {
	            //case 1: there are tasks to process, wake up threads
	            //the case should go first, since we want to wake up threads early, not till all are idle as in case 2
	            mtx_go.lock();
	            ready_go = true;
	            cv_go.notify_all(); //release threads to compute tasks
	            mtx_go.unlock();
	        }
	        else
	        {
	            mtx_go.lock();
	            if(global_num_idle == THREADS)
	            //case 2: every thread is waiting, guaranteed since mtx_go is locked
	            //since we are in else-branch, task_queue must be empty
	            {
	                global_end_label = true;
	                ready_go = true;
	                cv_go.notify_all(); //release threads to their looping
	            }
	            //case 3: else, some threads are still processing tasks, check in next round
	            mtx_go.unlock();
	        }
	        q_mtx.unlock();
	    }
	    //------------------------
	    for(int i=0; i<THREADS; i++) threads[i].join();
	}
	// ============== main logic end =================

	void run() {
		setRoot(tqueue());
		parallel_run();
	}

	virtual ~Worker(){
		delete (vector<TransT>*)transDB;
		delete (stack<TaskT*> *)task_queue;

		fd.close();
		for(int i=0; i<THREADS; i++)
			fouts[i].close();
		delete[] fouts;
	}
};

#endif /* WORKER_H_ */
