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

#ifndef GSPANUTILS_H_
#define GSPANUTILS_H_

#include "Trans.h"
#include "Pattern.h"
#include "timetrack.h"
#include "Task.h"
#include "Worker.h"
#include "Global.h"

#include <sstream>
#include <atomic>
#define USE_ID
#define USE_ASSERT

#ifdef USE_ID
atomic<int> ID(0);
#endif
bool directed = false;
bool vLabel_patt = true;

template <class T, class Iterator>
void tokenize (const char *str, Iterator iterator)
{
	istringstream is (str);
	copy (istream_iterator <T> (is), istream_iterator <T> (), iterator);
}

#endif /* GSPANUTILS_H_ */
