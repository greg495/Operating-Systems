//GROUP MEMBERS: Ian Gross, Greg McCutcheon, and Parker Slote

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <limits>
using namespace std;

#define NUM_CPU 1 //Number of processors available
#define T_CS 8 //Time in ms for a context switch
#define T_SLICE 84 //Time in ms for RR to move on to a new process

class process{
public:
	//main elements
	char proc_id;
	int init_arrival;
	int burst_time;
	int num_bursts;
	int io_time;
	
	//Tracking elements
	bool late_arrival;
	int num_bursts_executed;
	int context_wait;
	
	//holds various times of how long the process has been in a queue or CPU
	int IOtimeLeft;
	int CPUtime;
	int RRtimeLeft;
	
	//for data calculations
	int RQwaittime;
	int TTtotaltime;
	
	
	process(char id, int arrival, int burst, int num, int io) {
		//Constructor to initialize vars
		proc_id = id;
		init_arrival = arrival;
		burst_time = burst;
		num_bursts = num;
		io_time = io;
		late_arrival = false;
		num_bursts_executed = 0;
		IOtimeLeft = io_time;
		RQwaittime = 0;
		TTtotaltime = 0;
		CPUtime = burst_time;
		RRtimeLeft = -1;
	}
	
	process(void) {
	    //Constructor to initialize vars
    	proc_id = '_';
    	init_arrival = 0;
    	burst_time = 0;
    	num_bursts = 0;
    	io_time = 0;
		late_arrival = false;
		num_bursts_executed = 0;
		IOtimeLeft = io_time;
		RQwaittime = 0;
		TTtotaltime = 0;
		CPUtime = burst_time;
		context_wait = 0;
		RRtimeLeft = -1;
	}
	//functions for getting vars
	char get_proc_id(void){ return proc_id; }
	int get_init_arrival(void){ return init_arrival; }
	int get_burst_time(void){ return burst_time; }
	int get_num_bursts(void){ return num_bursts; }
	int get_io_time(void){ return io_time; }
	void burst_done(void){ num_bursts--; num_bursts_executed++;}
	void IOreset(void) { IOtimeLeft = io_time; }
	void RQ_time_pass(void){
		//Handles passage of time if process is on readyQ
		RQwaittime++;
		TTtotaltime++;
	}

	void CPU_time_pass(void){
		//Handles passage of time if process is on a CPU
		if (CPUtime > 0){
			CPUtime--;
			TTtotaltime++;
		}
	}

	void IO_time_pass(void){
		//Handles passage of time if process is on IO
		IOtimeLeft--;
	}
};

class CPU_class{
public:
	bool isCPUReady;
	//for the multiple CPU stuff
	int startTime;
	int timeLeft;
	int waitTime; //Time before the CPU can start working
	
	//the current process in the CPU
	process curProcess;
	
	void setCPUState(bool i){ isCPUReady = i; }
	bool getCPUState(){ return isCPUReady; }
	void setcurProcess(process p){ curProcess = p; }
	void time_pass(void) {
		curProcess.CPU_time_pass();
		timeLeft--;
	}
	void time_set(int t) { timeLeft = t; }
	int get_time_left(void) { return timeLeft; }
};



double Compute_Avg_CPU_Burst(vector<process> &q){
	double sum = 0.0;
	double count = 0.0;
	for (unsigned int y = 0; y < q.size(); y++){
		//for the number of busrts that each process much calculate
		sum += (q[y].burst_time * q[y].num_bursts);
		count += q[y].num_bursts;
	}
	
	return sum/count;
}


double Compute_Avg_Wait_Time(vector<process> &q){
	double sum = 0.0;
	double count = 0.0;
	for (unsigned int y = 0; y < q.size(); y++){
		sum += (double)q[y].RQwaittime;
		count += (double)q[y].num_bursts;
	}
	
	return sum/count;
}

double Compute_Avg_Turnaround_Time(vector<process> &q){
	double sum = 0.0;
	double count = 0.0;
	for (unsigned int y = 0; y < q.size(); y++){
		sum += q[y].get_num_bursts() * q[y].get_burst_time() + q[y].RQwaittime;
		count += q[y].num_bursts;
	}
	
	return sum/count;
}

double Compute_Avg_Turnaround_Time_RR(vector<process> &q){
	double sum = 0.0;
	double count = 0.0;
	for (unsigned int y = 0; y < q.size(); y++){
		sum += q[y].TTtotaltime;
		count += q[y].num_bursts;
	}
	
	return sum/count;
}

string queuePrint(vector<process> &q){
	string master = "[Q";
	if(q.empty()){
		master.append(" empty]");
	}
	else if(!q.empty()){
		unsigned int h = 0;
		for(h = 0; h < q.size(); h++){
			master.append(" ");
			stringstream ss;
			string s;
			ss << q[h].proc_id;
			ss >> s;
			master.append(s);
		}
		master.append("]");
	}
	return master;
}

void printQ(vector<process>* readyQ){
	//Prints the current values of readyQ
	unsigned int ii = 0;
	printf(" [Q");
	if (readyQ->size() == 0){
		printf(" empty]\n");
	}
	else{
		for (ii = 0; ii < readyQ->size(); ii++){
			printf(" %c", (*readyQ)[ii].get_proc_id());
		}
		printf("]\n");
	}
}



//--------------------------------------------------------------------------------------------------------------


void FCFS(vector<process> &p, unsigned int process_num, int numCPU, FILE* outfile, int t_cs){
	
	//Define the queues
	vector<process> readyQ; // Holds the processes ready to use the CPU
	vector<process> waitQ;	// Holds processes that are stuck on I/O. Get moved to readyQ when done
	vector<process> doneQ;	// Holds processes that have completed all of their bursts
	
	//Define and set the state of the CPUs as ready
	vector<CPU_class> CPU(numCPU);
	
	//track the number of context switches
	int context_switches = 0;
	
	int i = 0;
	unsigned int ii = 0;
	for(i = 0; i < numCPU; i++){
		CPU[i].setCPUState(true);
	}
	
	//track the number of context switches
	//t is the time value
	int t = 0;
	//have all the processes enter the ready queue if at time zero and enter a message
	
	printf("time %dms: Simulator started for FCFS %s\n", t, queuePrint(readyQ).c_str());
	
	for(ii = 0; ii < process_num; ii++){
		if(p[ii].init_arrival == 0){
			readyQ.push_back(p[ii]);
			printf("time %dms: Process %c arrived %s\n", t, readyQ.back().proc_id, queuePrint(readyQ).c_str());
			readyQ.back().context_wait += t_cs / 2;
		}
		else if (p[ii].init_arrival > 0){
			p[ii].late_arrival = true;
			waitQ.push_back(p[ii]);
		}
	}

	while(doneQ.size() != process_num){
		
		//Check to see if any late or items in the wait queue are read to me moved back to the readyQ
		for(unsigned int k = 0; k < waitQ.size(); k++){
			if(waitQ[k].init_arrival == 0 && waitQ[k].late_arrival == true){
				waitQ[k].late_arrival = false;
				readyQ.push_back(waitQ[k]);
				waitQ.erase(waitQ.begin() + k);
				printf("time %dms: Process %c arrived %s\n", t, readyQ.back().proc_id, queuePrint(readyQ).c_str());
				k--;
			}
		}
		
		//Check to see if any processes are done running
		for(int k = 0; k < numCPU; k++){
			//fprintf(outfile, "time %dms: Time LEFT %d\n", t, CPU[k].timeLeft);
			if(CPU[k].timeLeft == 0 && CPU[k].isCPUReady == false){
				
				//SEND TO WAIT Q and FREE THE CPU
				CPU[k].isCPUReady = true;
				CPU[k].curProcess.num_bursts_executed++;
				
				int bursts_left = CPU[k].curProcess.num_bursts - CPU[k].curProcess.num_bursts_executed;
				
				process tmp_p;
				
				if(bursts_left == 0){
					doneQ.push_back(CPU[k].curProcess);
					CPU[k].curProcess = tmp_p;
					printf("time %dms: Process %c terminated %s\n", t, doneQ.back().proc_id, queuePrint(readyQ).c_str());
					
					if(doneQ.size() == process_num){
						t += t_cs / 2;
						context_switches++;
						printf("time %dms: Simulator ended for FCFS\n\n", t);
						
						//compute stats
						fprintf(outfile, "Algorithm FCFS\n");
						fprintf(outfile, "-- average CPU burst time: %.2f ms\n", Compute_Avg_CPU_Burst(p));
						fprintf(outfile, "-- average wait time: %.2f ms\n", Compute_Avg_Wait_Time(doneQ));
						fprintf(outfile, "-- average turnaround time: %.2f ms\n", Compute_Avg_Turnaround_Time(doneQ));
						fprintf(outfile, "-- total number of context switches: %d\n", context_switches);
						fprintf(outfile, "-- total number of preemptions: %d\n", 0);
						
						return;
					}
				}
				else{
					printf("time %dms: Process %c completed a CPU burst; %d to go %s\n", t, CPU[k].curProcess.proc_id, bursts_left, queuePrint(readyQ).c_str());
					
					waitQ.push_back(CPU[k].curProcess);
					CPU[k].curProcess = tmp_p;
					waitQ.back().IOtimeLeft = waitQ.back().io_time;
					int t_till_free = t + waitQ.back().io_time;
					printf("time %dms: Process %c blocked on I/O until time %dms %s\n", t, waitQ.back().proc_id, t_till_free, queuePrint(readyQ).c_str());
					waitQ.back().TTtotaltime += t_cs;
				}
				CPU[k].waitTime += t_cs;
				context_switches++;
				
			}
		}
		
		for(unsigned int k = 0; k < waitQ.size(); k++){
			if(waitQ[k].IOtimeLeft == 0 && waitQ[k].late_arrival == false){
				//PRINT AND SEND TO READY Q				
				readyQ.push_back(waitQ[k]);
				waitQ.erase(waitQ.begin() + k);
				k--;
				printf("time %dms: Process %c completed I/O %s\n", t, readyQ.back().proc_id, queuePrint(readyQ).c_str());
				readyQ.back().context_wait += t_cs / 2;
				//context_switches += 0.5;
			}
		}
		
		if(!readyQ.empty()){
			//check to see if any cpu's are available
			for(int k = 0; k < numCPU; k++){
				for (unsigned int p_v = 0; p_v < readyQ.size(); p_v++){
					if(CPU[k].isCPUReady && !readyQ.empty() && CPU[k].waitTime == 0 && readyQ[p_v].context_wait == 0){
						CPU[k].setCPUState(false);
						CPU[k].setcurProcess(readyQ.front());
						CPU[k].startTime = t;
						
						CPU[k].timeLeft = CPU[k].curProcess.burst_time;
						readyQ.erase(readyQ.begin());
						printf("time %dms: Process %c started using the CPU %s\n", t, CPU[k].curProcess.proc_id, queuePrint(readyQ).c_str());
					}
				}
			}
		}
		
		
		//----------------------------------------------------------------------------------------------------
		
		//increment/decrement each of the necessary items
		//increment work CPUs
		for(int k = 0; k < numCPU; k++){
			//time until cpu is ready
			if(CPU[k].waitTime > 0){
				CPU[k].waitTime--;
			}
			if(CPU[k].isCPUReady == false && CPU[k].waitTime == 0){
				CPU[k].timeLeft--;
				CPU[k].curProcess.TTtotaltime++;
			}
		}
			
		for(unsigned int k = 0; k < readyQ.size(); k++){
			//TIME THE ELEMENT HAS BEEN WAITING ++, important for calculations
			
			//When it gets out of I/0, it must wait 4 seconds, track that here
			if(readyQ[k].context_wait > 0){
				readyQ[k].context_wait--;
			}
			else{
				readyQ[k].RQwaittime++;
				readyQ[k].TTtotaltime++;
			}
		}
			
		//increment values in wait queue, or decrement arrival time
		for(unsigned int k = 0; k < waitQ.size(); k++){
			if(waitQ[k].init_arrival > 0 && waitQ[k].late_arrival == true){
				waitQ[k].init_arrival--;
			}
			else if(waitQ[k].IOtimeLeft > 0 && waitQ[k].late_arrival == false){
				waitQ[k].IOtimeLeft--;
			}
		}
		//increment time interval by 1
		t++;
	}
	//Should exit in loop, rather than here
	printf("time %dms: Simulator ended for FCFS - INCORRECTLY\n", t);
	return;
};



//--------------------------------------------------------------------------------------------------------------



void SJF_insert(vector<process> * p, process curr_process, int t){
	//Inserts a process into a vector in the style of SJF
	if ((p->size() == 0) || (curr_process.get_burst_time)() > (*p)[p->size() - 1].get_burst_time()){
		//If there is nothing there, or it's the biggest one, just add it to the end
		p->push_back(curr_process);
	}
	else {
		//Else, search through and find where to insert it
		vector<process>::iterator it;
		for (it = p->begin(); it != p->end(); it++)
		{
			if (curr_process.get_burst_time() < it->get_burst_time()){
				p->insert(it, curr_process);
				break;
			}
		}
	}
}

void SJF(vector<process> * p, unsigned int process_num, FILE* outfile){
	//*p holds all the processes
	//Define the queues
	vector<process> readyQ; // Holds the processes ready to use the CPU
	vector<process> waitQ;	// Holds processes that are stuck on I/O. Get moved to readyQ when done
	vector<process> doneQ;	// Holds processes that have completed all of their bursts
	
	//track the number of context switches
	int context_switches = 0;

	//Define and set the state of the CPUs as ready
	vector<CPU_class> CPU(NUM_CPU);

	process curr_process;
	unsigned int i = 0; //for loop indices
	int t = 0; //time value in ms

	for (i = 0; i < NUM_CPU; i++){
		//CPU[i].setCPUState(true);
		CPU[i].time_set(T_CS / 2); //Stuff doesn't get to the CPU until half the time of a context switch
	}
	//context_switches += 0.5;

	//This code is repeated several times to print the contents of readyQ
	printf("time %dms: Simulator started for SJF", t);
	printQ(&readyQ);

	for (t = 0; doneQ.size() != p->size(); t++){
		//for (t = 0; t < 200; t++){ //Temporary for building the code
		for (i = 0; i < process_num; i++){
			curr_process = (*p)[i];
			if (curr_process.get_init_arrival() == t){ //If arrival time is here, add it to the ready queue
				SJF_insert(&readyQ, curr_process, t);
				printf("time %dms: Process %c arrived", t, curr_process.get_proc_id());
				printQ(&readyQ);

			}
		}
		for (i = 0; i < readyQ.size(); i++){
			readyQ[i].RQ_time_pass();
		}

		for (i = 0; i < NUM_CPU; i++){
			if (CPU[i].get_time_left() == 0 && CPU[i].curProcess.get_proc_id() != '_'){
				//If a burst has just finished, remove it from the CPU and do a context switch
				CPU[i].curProcess.burst_done();
				//If burst is done, check if there are more bursts to do
				if (CPU[i].curProcess.get_num_bursts() > 0){ //If more to do, put the process on IO
					printf("time %dms: Process %c completed a CPU burst; %d to go", t, CPU[i].curProcess.get_proc_id(), CPU[i].curProcess.get_num_bursts());
					printQ(&readyQ);
					printf("time %dms: Process %c blocked on I/O until time %dms", t, CPU[i].curProcess.get_proc_id(), t + CPU[i].curProcess.get_io_time());
					printQ(&readyQ);
					waitQ.push_back(CPU[i].curProcess);
					waitQ.back().IOtimeLeft = waitQ.back().io_time;
				}
				else { //Else, terminate (IO is only done between bursts)
					printf("time %dms: Process %c terminated", t, CPU[i].curProcess.get_proc_id());
					printQ(&readyQ);
					doneQ.push_back(CPU[i].curProcess);
				}
				process temp;
				CPU[i].setcurProcess(temp); //Clear out curProcess
				CPU[i].time_set(T_CS); //Context switch stall before new process
				context_switches++;
			}
			if (CPU[i].curProcess.get_proc_id() == '_' && readyQ.size() == 0){
				//Defaults time left to 4ms if no processes on readyQ or the CPU
				CPU[i].time_set(T_CS / 2);
			}
			if (CPU[i].timeLeft <= 0 && readyQ.size() > 0){
				//If there is a ready process and an open CPU, take it off the readyQ and put it in the CPU
				CPU[i].setcurProcess(readyQ[0]);
				readyQ.erase(readyQ.begin());
				printf("time %dms: Process %c started using the CPU", t, CPU[i].curProcess.get_proc_id());
				printQ(&readyQ);
				CPU[i].time_set(CPU[i].curProcess.get_burst_time());
			}
			CPU[i].time_pass();
		}

		for (i = 0; i < waitQ.size(); i++){
			//Vector erase needs iterator as input; (waitQ.begin() + i) is that
			//Accounts for processes on IO
			if (waitQ[i].get_proc_id() != '_' && waitQ[i].IOtimeLeft == 0){
				//Checks if the process is valid first, then if finished
				waitQ[i].IOreset();
				SJF_insert(&readyQ, waitQ[i], t);
				printf("time %dms: Process %c completed I/O", t, waitQ[i].get_proc_id());
				printQ(&readyQ);

				waitQ.erase(waitQ.begin() + i);
				i--;
			}
			else{
				waitQ[i].IO_time_pass();
			}
		}
	}
	t += 3;

	//for calculations

	for(i = 0; i < doneQ.size(); i++){
		doneQ[i].num_bursts = doneQ[i].num_bursts_executed;
	}

	printf("time %dms: Simulator ended for SJF\n\n", t);
	fprintf(outfile, "Algorithm SJF\n");
	fprintf(outfile, "-- average CPU burst time: %.2f ms\n", Compute_Avg_CPU_Burst(*p));
	fprintf(outfile, "-- average wait time: %.2f ms\n", Compute_Avg_Wait_Time(doneQ));
	fprintf(outfile, "-- average turnaround time: %.2f ms\n", Compute_Avg_Turnaround_Time(doneQ));
	fprintf(outfile, "-- total number of context switches: %d\n", context_switches);
	fprintf(outfile, "-- total number of preemptions: %d\n", 0);
}




//--------------------------------------------------------------------------------------------------------------







void RR(vector<process> &p, unsigned int process_num, int numCPU, FILE* outfile, int t_cs){
	
	//Define the queues
	vector<process> readyQ; // Holds the processes ready to use the CPU
	vector<process> waitQ;	// Holds processes that are stuck on I/O. Get moved to readyQ when done
	vector<process> doneQ;	// Holds processes that have completed all of their bursts
	
	//Define and set the state of the CPUs as ready
	vector<CPU_class> CPU(numCPU);
	int i = 0;
	unsigned int ii = 0;
	for(i = 0; i < numCPU; i++){
		CPU[i].setCPUState(true);
		//CPU[i].waitTime = 4;
	}
	
	//t is the time value
	int t = 0;
	//t_slice_rem keeps track of the time left in the slice
	int t_slice_rem = T_SLICE;
	//have all the processes enter the ready queue if at time zero and enter a message
	
	//track the number of context switches
	int context_switches = 0;
	int preemptions = 0;
	
	printf("time %dms: Simulator started for RR %s\n", t, queuePrint(readyQ).c_str());
	
	for(ii = 0; ii < process_num; ii++){
		//set RRtimeLeft for each value. This could be done before, but whatever
		p[ii].RRtimeLeft = p[ii].burst_time;
		if(p[ii].init_arrival == 0){
			readyQ.push_back(p[ii]);
			printf("time %dms: Process %c arrived %s\n", t, readyQ[ii].proc_id, queuePrint(readyQ).c_str());
			readyQ.back().context_wait += t_cs / 2;
		}
		else if (p[ii].init_arrival > 0){
			p[ii].late_arrival = true;
			waitQ.push_back(p[ii]);
		}
	}
	

	while(doneQ.size() != process_num){
		//Check to see if any late or items in the wait queue are read to me moved back to the readyQ
		for(unsigned int k = 0; k < waitQ.size(); k++){
			if(waitQ[k].init_arrival == 0 && waitQ[k].late_arrival == true){
				waitQ[k].late_arrival = false;
				readyQ.push_back(waitQ[k]);
				waitQ.erase(waitQ.begin() + k);
				printf("time %dms: Process %c arrived %s\n", t, readyQ.back().proc_id, queuePrint(readyQ).c_str());
				k--;
			}
		}
		
		
		//Check to see if any processes are done running
		for(int k = 0; k < numCPU; k++){
			if(CPU[k].curProcess.RRtimeLeft == 0 && CPU[k].isCPUReady == false){
				//SEND TO WAIT Q and FREE THE CPU
				CPU[k].isCPUReady = true;
				CPU[k].curProcess.num_bursts_executed++;
				int bursts_left = CPU[k].curProcess.num_bursts - CPU[k].curProcess.num_bursts_executed;
				process tmp_p;
				
				if(bursts_left == 0){
					doneQ.push_back(CPU[k].curProcess);
					CPU[k].curProcess = tmp_p;
					printf("time %dms: Process %c terminated %s\n", t, doneQ.back().proc_id, queuePrint(readyQ).c_str());
					
					if(doneQ.size() == process_num){
						t += t_cs / 2;
						context_switches++;
						printf("time %dms: Simulator ended for RR\n", t);
						
						//compute stats
						fprintf(outfile, "Algorithm RR\n");
						fprintf(outfile, "-- average CPU burst time: %.2f ms\n", Compute_Avg_CPU_Burst(p));
						fprintf(outfile, "-- average wait time: %.2f ms\n", Compute_Avg_Wait_Time(doneQ));
						fprintf(outfile, "-- average turnaround time: %.2f ms\n", Compute_Avg_Turnaround_Time_RR(doneQ));
						fprintf(outfile, "-- total number of context switches: %d\n", context_switches);
						fprintf(outfile, "-- total number of preemptions: %d\n", preemptions);	
						
						return;
					}
				}
				else{
					printf("time %dms: Process %c completed a CPU burst; %d to go %s\n", t, CPU[k].curProcess.proc_id, bursts_left, queuePrint(readyQ).c_str());
					
					waitQ.push_back(CPU[k].curProcess);
					CPU[k].curProcess = tmp_p;
					waitQ.back().IOtimeLeft = waitQ.back().io_time;
					int t_till_free = t + waitQ.back().io_time;
					printf("time %dms: Process %c blocked on I/O until time %dms %s\n", t, waitQ.back().proc_id, t_till_free, queuePrint(readyQ).c_str());
					waitQ.back().TTtotaltime += t_cs;
				}
				CPU[k].waitTime += t_cs;
				context_switches++;
			}
		}
		
		
		
		//check for if time slice has expired
		for(int k = 0; k < numCPU; k++){
			if(CPU[k].curProcess.RRtimeLeft != 0 && t_slice_rem == 0 && CPU[k].isCPUReady == false && readyQ.empty()){
				t_slice_rem = T_SLICE;
				printf("time %dms: Time slice expired; no preemption because ready queue is empty %s\n", t, queuePrint(readyQ).c_str());
			}
			else if(CPU[k].curProcess.RRtimeLeft != 0 && t_slice_rem == 0 && CPU[k].isCPUReady == false){
				
				//SEND TO WAIT Q and FREE THE CPU
				CPU[k].isCPUReady = true;
				process tmp_p;				
				readyQ.push_back(CPU[k].curProcess);
				CPU[k].curProcess = tmp_p;
				printf("time %dms: Time slice expired; process %c preempted with %dms to go %s\n", t, readyQ.back().proc_id,  readyQ.back().RRtimeLeft, queuePrint(readyQ).c_str());
				preemptions++;
				CPU[k].waitTime += t_cs;
				context_switches++;
				readyQ[k].RQwaittime -= t_cs;
			}
		}
		
		
		if(!readyQ.empty()){
			//check to see if any cpu's are available
			for(int k = 0; k < numCPU; k++){
				for (unsigned int p_v = 0; p_v < readyQ.size(); p_v++){
					if(CPU[k].isCPUReady && !readyQ.empty() && CPU[k].waitTime == 0 && readyQ[p_v].context_wait == 0){
						CPU[k].setCPUState(false);
						CPU[k].setcurProcess(readyQ.front());
						CPU[k].startTime = t;
						readyQ.erase(readyQ.begin());
						printf("time %dms: Process %c started using the CPU %s\n", t, CPU[k].curProcess.proc_id, queuePrint(readyQ).c_str());
						t_slice_rem = T_SLICE;
					}
				}
			}
		}

		for(unsigned int k = 0; k < waitQ.size(); k++){
			if(waitQ[k].IOtimeLeft == 0 && waitQ[k].late_arrival == false){
				//PRINT AND SEND TO READY Q
				readyQ.push_back(waitQ[k]);
				waitQ.erase(waitQ.begin() + k);
				k--;
				printf("time %dms: Process %c completed I/O %s\n", t, readyQ.back().proc_id, queuePrint(readyQ).c_str());
				readyQ.back().context_wait += t_cs / 2;
				readyQ.back().RRtimeLeft = readyQ.back().burst_time;
			}
		}
		
		//----------------------------------------------------------------------
		
		//increment/decrement each of the necessary items
		//increment work CPUs
		for(int k = 0; k < numCPU; k++){
			if(CPU[k].waitTime > 0){
				CPU[k].waitTime--;
			}
			//check to see if there is a process in the cpu
			if(CPU[k].isCPUReady == false && CPU[k].waitTime == 0){
				CPU[k].curProcess.RRtimeLeft--;
				CPU[k].curProcess.TTtotaltime++;
			}
		}
			
		for(unsigned int k = 0; k < readyQ.size(); k++){
			//TIME THE ELEMENT HAS BEEN WAITING ++, important for calculations
			//When it gets out of I/0, it must wait 4 seconds, track that here
			if(readyQ[k].context_wait > 0){
				readyQ[k].context_wait--;
			}
			else{
				readyQ[k].RQwaittime++;
				readyQ[k].TTtotaltime++;
			}
		}
		
		//increment values in wait queue, or decrement arrival time
		for(unsigned int k = 0; k < waitQ.size(); k++){
			if(waitQ[k].init_arrival > 0 && waitQ[k].late_arrival == true){
				waitQ[k].init_arrival--;
			}
			else if(waitQ[k].IOtimeLeft > 0 && waitQ[k].late_arrival == false){
				waitQ[k].IOtimeLeft--;
			}
		}
		
		//decrement the time until the time slice expires
		if(t_slice_rem > 0){
			t_slice_rem--;
		}
		//increment time interval by 1
		t++;
	}
	
	//Should exit in loop, rather than here
	printf("time %dms: Simulator ended for FCFS - INCORRECTLY\n", t);
	
	return;
};



//--------------------------------------------------------------------------------------------------------------





std::vector<process> parse(char* file_name){
    vector<process> processes;
    process curr_process;
    char temp_c;
    char* temp_s = (char*) calloc(30, sizeof(char));
    int num_processes = 0; //The number of processes in the input file, which disagrees with the given number in the assignment

    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
      fprintf(stderr, "Can't open input file\n");
      exit(EXIT_FAILURE); 
    }
    
    //Parse the file: ignore lines starting with #
    temp_c = fgetc(file);
    //putchar(temp_c);
    while (temp_c != EOF){
        if (temp_c == '#'){
            //If a #, fast forward to the next line
            //temp_c = fgetc(file);
            while (temp_c != EOF && temp_c != '\n'){
                temp_c = fgetc(file);
                //putchar(temp_c);
            }
            if (temp_c == EOF){
                break;
            }
        }
        else{
            //Else, parse as <proc-id>|<initial-arrival-time>|<cpu-burst-time>|<num-bursts>|<io-time>
            ungetc(temp_c, file); //Puts temp_c back on the stream to be read again
            fscanf(file, "%s", temp_s); 
            //fscanf shits the bed when used directly; reading the whole line then using sscanf seems to work
            sscanf (temp_s, "%c|%d|%d|%d|%d", &(curr_process.proc_id), &(curr_process.init_arrival), 
                &(curr_process.burst_time), &(curr_process.num_bursts), &(curr_process.io_time));
            processes.push_back(curr_process);
            num_processes++;
        }
        temp_c = fgetc(file);
    }
    fclose (file);	
	
	
	//FIX INPUT ERROR - It takes the last 2 elements and puts in vector twice
	if (processes[num_processes - 1].proc_id == processes[num_processes - 2].proc_id){
		processes.erase(processes.end());
		num_processes--;
	}
	
    return processes;
}

//--------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc > 3){
        fprintf(stderr, "Too many arguments\n");
        exit(EXIT_FAILURE); 
    }
    else if (argc < 3){
        fprintf(stderr, "ERROR: Invalid arguments\nUSAGE: ./a.out <input-file> <stats-output-file>\n");
        exit(EXIT_FAILURE); 
    }
	
	//Read the input file into a vector of processes
    vector<process> processes = parse(argv[1]);
	
	
	FILE* outfile = fopen(argv[2], "w+");
    if (outfile == NULL) {
      fprintf(stderr, "Can't open output file\n");
      exit(EXIT_FAILURE); 
    }
	
	int m = 1;
	int n = processes.size();
	int t_cs = 8; //The amount of time it takes to perform a context switch
	
	
	FCFS(processes, n, m, outfile, t_cs);
	SJF(&processes, n, outfile);
	RR(processes, n, m, outfile, t_cs);
	
	fclose(outfile);
}