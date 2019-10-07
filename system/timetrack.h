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

#ifndef TIMETRACH_H
#define TIMETRACH_H
#include <sys/time.h>
#include <unistd.h>  

#define microsec 1000000.0

class TimeTracker {
private:
	struct timeval start_time;
	struct timeval stop_time;
	double total_time;
	bool running;
public:
	TimeTracker(){
		running = true;
		total_time = 0.0;
	}

	void start() {
		gettimeofday(&start_time, (struct timezone*) 0);
		running = true;
	}

	double stop() {
		double st, en;
		if (!running)
			return -1;
		else {
			gettimeofday(&stop_time, (struct timezone*) 0);
			st = start_time.tv_sec + (start_time.tv_usec / microsec);
			en = stop_time.tv_sec + (stop_time.tv_usec / microsec);
			running = false;
			total_time +=  (double) (en - st);
			return total_time;
		}
	}

	void reset() {
		total_time = 0.0;
	}

};

#endif

