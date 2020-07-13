## PrefixFPM: A Parallel Framework for Mining Frequent Patterns
Following divide and conquer, this system treats each pattern to examine and extend as a task, and parallelizes all tasks as much as possible following the idea of divide and conquer. This allows it to use all CPU cores in a machine to mine frequent patterns over big data.

There are 3 applications on top of our framework. Here are the folder structures:

* system: the general-purpose programming interface
* prefixspan: the PrefixSpan parallel version developed on top for sequence mining
* gspan: the gSpan parallel version developed on top for subgraph mining
* sleuth: the Sleuth parallel version developed on top for subtree mining

The applicatons are adapted from:

* PrefixSpan: http://chasen.org/~taku/software/prefixspan/prefixspan-0.4.tar.gz
* gSpan: https://github.com/rkwitt/gboost/tree/master/src-gspan
* Sleuth: http://www.cs.rpi.edu/~zaki/www-new/pmwiki.php/Software/Software#toc19

Note that OpenMP is used and should be enabled in your Makefile. There are 3 run_XXX.cpp files for the 3 applications above. To compile each one, you may run "cp run_XXX.cpp run.cpp" and then run "make". The compiled "run" program can be renamed to a meaningful name for later use.

A sample command to run each program is put at the end of each each run_XXX.cpp file, which reads data from the sample data that we put in the folder of each application.

The following is the complete command:

./run -s [support_threshold] -i [input_file] -n [thread_number] -t [task_parallelism_threshold] -T [PDB_parallelism_threshold]

### Contact
Da Yan: https://yanlab19870714.github.io/yanda

Email: yanda@uab.edu

### Contributors
Wenwen Qu

Da Yan
