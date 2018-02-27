CPU & Memory Simulation
CPU forks to create a child process which executes the memory.
CPU fetches instructions from the child process using a pipe.


File Descriptions:

sample5.txt - A sample input file containing the user program input for the Memory. It should output my name "SONIA" 5 times
before exiting.

cpu.cc - C++ source file for the CPU and Memory simulation. 



Compilation instructions:

g++ -o cpu cpu.cc




Execution instructions:
The executable file takes two arguments: filename and interrupt timeout value

./cpu sample1.txt 30