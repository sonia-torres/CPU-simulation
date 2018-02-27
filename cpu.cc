#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>
#include <algorithm>

using namespace std;

class Registers {
public:
	int AC;
	int PC;
	int IR;
	int X;
	int Y;
	int SP;
};

enum Mode {r, w};
enum cpuMode {user, kernel};

class Memory {
private:
	int mem[2000];
public:
	Mode mode;	
	
	Memory() {
		mode = r;
	}

	int read(int addr) {
		return mem[addr];
	}

	void write(int addr, int val) {
		mem[addr] = val;
	}
};
//Function Headers ************************
bool loader(char* filename, Memory &memory);
void cpu();
void memory(char* filename);
void push(int val);
int pop();
void memWrite(int addr, int val);
int memRead(int addr);
//Global variables ************************
int PARENT[2], CHILD[2]; 
const int exit_code = -1;
const int write_code = -2;
Registers registers;
int TIMER;
cpuMode CPU_MODE = user;
//*****************************************
int main(int argc, char** argv) {
	int pipe1, pipe2, forkval;
	
	if(argc < 3){
		cout << "Missing arguments. Must provide filename and timer interrupt value." << endl;
		return 0;
	}
	pipe1 = pipe(PARENT);
	pipe2 = pipe(CHILD);
	TIMER = atoi(argv[2]);
	forkval = fork();

	if(forkval == 0) {
	//we are child Memory
    		memory(argv[1]);    
	}
	
	else{
	//we are parent CPU
		cpu();
	}
	return 0;
}

void memWrite(int addr, int val) {
	write(PARENT[1], &write_code, sizeof(write_code)); //change mode to write
	write(PARENT[1], &addr, sizeof(addr)); // addr
	write(PARENT[1], &val, sizeof(val)); //val
}

int memRead(int addr) {
	if(addr >= 1000 && CPU_MODE != kernel) {
		cout << "ERROR: Access Denied. Cannot access address " << addr << " in user mode"<< endl;
		write(PARENT[1], &exit_code, sizeof(exit_code)); //send exit signal to child
		exit(EXIT_FAILURE);
	}
	write(PARENT[1], &addr, sizeof(addr));
	int val;
	read(CHILD[0], &val, sizeof(val));
	return val;
}

void push(int val) {	
	memWrite(registers.SP, val);
	registers.SP--;
}
int pop() {
	registers.SP++;
	int val = memRead(registers.SP);
	return val;
}

void switchMode() {
	if(CPU_MODE == kernel) {
		//switch to user mode
		//registers.Y = pop();
		//registers.X = pop();
		//registers.AC = pop();
		//registers.IR = pop();
		registers.PC = pop();
		registers.SP = pop();
		CPU_MODE = user;
	}
	else {//switch to kernel mode
		int tempSP = registers.SP;
		registers.SP = 1999;
		push(tempSP);
		push(registers.PC);
		//push(registers.IR);
		//push(registers.AC);
		//push(registers.X);
		//push(registers.Y);
		CPU_MODE = kernel;
	}
}

void cycler(int &cycle) {
	cycle++;
	if(cycle >= TIMER) {
		if(CPU_MODE != kernel) {
			cycle = 0;
			switchMode();
			registers.PC = 1000;
		}
	}
}
bool loader(char*  filename, Memory &memory) {
	ifstream inFile;
	inFile.open(filename);
	string line;
	if(!inFile) {
		cout << "Unable to open file " << filename << endl;
		return false;
	}
	stringstream ss;
	int mempointer = 0;
	int val;
	while(getline(inFile, line)) {
		if(!line.empty()) {
			if(line[0] == '.')
			{
				line = line.substr(1, line.length());
				ss << line;
				ss>>val;
				mempointer = val;
				ss.str("");
				ss.clear();
			}
			else
			{
				if(line[0] == ' ' || line[0] == '/') //check if it is a comment
					continue;
				else {
					ss<<line;
					ss>>val;
					memory.write(mempointer, val);
					ss.str("");
					ss.clear();
					mempointer++;
				}
			}
		}
	}

	inFile.close();
	return true;
}

void cpu() {
	registers.PC = registers.IR = registers.AC = registers.X = registers.Y = 0;
	registers.SP = 999;
	int addr;
	int cycle = 0;
	srand(time(NULL));
	//get first instruction
	registers.IR = memRead(registers.PC);
		
	while(registers.IR !=  50)
	{
		registers.PC++; //increment program counter
		//execute the instruction
		switch(registers.IR)
		{
			case 1: //load value
				registers.AC = memRead(registers.PC);
				registers.PC++;
				break;
			case 2: //load addr
				addr = memRead(registers.PC);
				registers.PC++;
				registers.AC = memRead(addr);
				break;
			case 3: //loadind addr
				addr = memRead(registers.PC);
				registers.PC++;
				registers.AC = memRead(memRead(addr));
				break;
			case 4: //loadidxx addr
				addr = memRead(registers.PC);
				registers.PC++;
				addr += registers.X;
				registers.AC = memRead(addr);
				break;
			case 5: //loadidxy addr
				addr = memRead(registers.PC);
				registers.PC++;
				addr += registers.Y;
				registers.AC = memRead(addr);
				break;
			case 6: //load spx
				addr = registers.X + registers.SP;
				registers.AC = memRead(addr);
				break;
			case 7: //store addr
				addr = memRead(registers.PC);
				registers.PC++;
				memWrite(addr, registers.AC);
				break;
			case 8: //get random int
				registers.AC = (rand() % 100) + 1;
				break;
			case 9: // put port
				addr = memRead(registers.PC);
				registers.PC++;

				if(addr == 1)
					cout << registers.AC;
				else
					cout << static_cast<char>(registers.AC);
				break;
			case 10: //addx
				registers.AC += registers.X;
				break;
			case 11: //addy
				registers.AC += registers.Y;
				break;
			case 12: //subx
				registers.AC -= registers.X;
				break;
			case 13: //suby
				registers.AC -= registers.Y;
				break;
			case 14: //CopyToX
				registers.X = registers.AC;
				break;
			case 15: //copyFromX
				registers.AC = registers.X;
				break;
			case 16: // CopyToY
				registers.Y = registers.AC;
				break;
			case 17: //copyFromY
				registers.AC = registers.Y;
				break;
			case 18: //copyToSP
				registers.SP = registers.AC;
				break;
			case 19: //copyFromSP
				registers.AC = registers.SP;
				break;
			case 20: //jump addr
				addr = memRead(registers.PC);
				registers.PC++;
				registers.PC = addr;
				break;
			case 21: //jumpIfEqual addr
				if(registers.AC == 0) {
					addr = memRead(registers.PC);
					registers.PC++;
					registers.PC = addr;
				}
				else {
					registers.PC++; //discard value
				}
				break;
			case 22: //jumpIfNotEqual addr
				if(registers.AC != 0) {
					addr = memRead(registers.PC);
					registers.PC++;
					registers.PC = addr;
				}
				else {
					registers.PC++; //discard value
				}
				break;
			case 23: //call addr	
				addr = memRead(registers.PC);
				registers.PC++;
				push(registers.PC);
				registers.PC = addr; //jump to addr
				break;
			case 24: //ret
				addr = pop();
				registers.PC = addr; //jump to addr
				break;
			case 25: //incX
				registers.X++;
				break;
			case 26: //decX
				registers.X--;
				break;
			case 27: //push
				push(registers.AC);
				break;
			case 28: //pop
				registers.AC = pop();
				break;
			case 29: //Int
				//we perform system call
				//disable interrupts during interrupt handling 
				if(CPU_MODE != kernel) {
					switchMode();
					registers.PC = 1500;
				}
				break;
			case 30: //IRet
				//we return from system call
				switchMode();
				break;
			default: cout << "Unknown instruction " << registers.IR << endl;
				break;
		}
		cycler(cycle); //check if we are on timeout
		//fetch the next instruction
		registers.IR = memRead(registers.PC);
		//cout << "Fetched instruction " << registers.IR << " at memory " << registers.PC << endl;
	}
	
	write(PARENT[1], &exit_code, sizeof(exit_code)); //send exit signal to child
}

void memory(char* filename) {
	Memory memory;
	int req;
	int res;
	int val;
	if(loader(filename, memory)) //load user program into memory
	{
		read(PARENT[0],&req, sizeof(req));
		while(req != exit_code)
		{
			if(req == write_code) //change mode of memory
			{
				memory.mode = w;
			}
			else if(memory.mode == r) {
				res = memory.read(req);
				write(CHILD[1], &res, sizeof(res));	
			}
			else {//write mode 
				read(PARENT[0], &val, sizeof(val)); //get val to be written
				memory.write(req, val);
				memory.mode = r; // back to read mode
			}

			read(PARENT[0], &req, sizeof(req));
		}
	}
	else {
		//exit parent process on failure
		res = 50;
		write(CHILD[1], &res, sizeof(res));
	}
}
