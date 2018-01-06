//Greg McCutcheon, Ian Gross, Ben Krowitz

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm> 
using namespace std;

#define LINE_SIZE 32
#define LINE_NUM 8 //Dimensions of the memory storage
#define FRAME_SIZE 3

char memory[LINE_NUM*LINE_SIZE]; //Constant size memory for the program

vector<char> v; //For defrag. Must be global, or it will be deallocated before the contents can be read.

//Class declaration for each process:
class process{
    public:
        char id; //uppercase letter denoting the name of the process
        int mem_needed; //Size of the memory needed
        vector< pair<int, int> > arrivals; //The different arrivals of this process; first is arrival time, second is run time
        process( char id_temp, int mem_needed_temp, vector< pair<int, int> > arrivals_temp){
            //Constructor to initialize vars
            id = id_temp;
            mem_needed = mem_needed_temp;
            arrivals = arrivals_temp;
        }
        process(){
            //Initialize vars to default values
            id = '_';
            mem_needed = 0;
            //arrivals is empty by default
        }
};


void print_mem() //Print the values currently in memory
{
    int i = 0;
    
    for (i = 0; i < LINE_SIZE; i++) { cout << '='; }
    cout << '\n';
    
    for (i = 0; i < LINE_NUM*LINE_SIZE; i++){
        cout << memory[i];
        if (i%LINE_SIZE == LINE_SIZE-1)
            cout << '\n';
    }
    
    for (i = 0; i < LINE_SIZE; i++){ cout << '='; }
    cout << '\n';
}

int defrag() //Defragments memory to condense the open spaces
{
    int i = 0, j = 0;
    int switches = 0; //Counts number of frame moves
    unsigned int k = 0;
    bool add = true; //keeps track of whether to add the current id to v
    char temp_c = '.';
    v.clear();
    for (i = 0; i < LINE_NUM*LINE_SIZE; i++){
        if (memory[i] == '.'){ //If an open spot
            for(j = i+1; j < LINE_NUM*LINE_SIZE; j++){
                if (memory[j] != '.') //Find the next spot that isn't open
                {   
                    //memory[j] is the frame we should switch
                    //Only add new processes:
                    add = true;
                    for (k = 0; k < v.size(); k++){
                        if (v[k] == memory[j]){
                            add = false;
                        }
                    }
                    if (add){
                        v.push_back(memory[j]);
                    }
                    //Move the filled spot up to the open one
                    temp_c = memory[j];
                    memory[j] = memory[i];
                    memory[i] = temp_c;
                    switches++;
                    break;
                }
            }
        }
    }
    return switches;
}

int place_process_nf(char id, int mem_needed, bool contiguous) //Place a process in memory in the method of next-fit, with input for contiguous or not
{ 
/*  Program must start from the end of the last process placed, and wrap around at the end until it reaches this spot.
    Thus, mod_i = (i + start) % SIZE is the real index for this function.
    Return value tells the program what to do next:
    0 means success, continue as normal
    1 means adding failed, but can be fixed with defragmentation
    2 means even defragmentation cannot allocate enough memory; time to skip the process and move on*/
    static int start = 0; //Keeps track of from where to start scanning the memory (end of the last-placed process)
    const int SIZE = LINE_NUM*LINE_SIZE;
    int i = 0, counter = 0; //For loop indices
    int it = i; //Saves a previous value of i
    int mod_i = (i + start) % SIZE; //The real index
    int mem_left = 0; //Counts the total amount of memory open to determine if defrag will help
    bool size_check = false; //Marks if the memory is being checked for a large enough space, as opposed to trying to find a space

    //Search through memory for an open spot:
    while ( (i < SIZE) || (memory[mod_i] == '.') ){ //Goes all the way through memory, but will finish checking the open area it's on
        mod_i = (i + start) % SIZE;
        if (mod_i == 0){
            size_check = false;
            counter = 0; //Processes cannot wrap from the end of memory to the beginning, so counter must reset
        }
        if (memory[mod_i] == '.'){ //If spot is open, check if it's big enough; increment counter for each succesive '.'
            if (!size_check){ //If this is the beginning of a new open spot, save the position
                it = mod_i;
            }
            size_check = true;
            counter++;
            mem_left++;
        }
        else if (contiguous){ 
            //If the scheme is contiguous and spot closed, turn off checking and reset counter
            //If non-contiguous and spot closed, do nothing
            size_check = false;
            counter = 0;
        }
        
        if (counter >= mem_needed){ //Success! Spot has been found!
            counter = 0;
            for (i = it; i < LINE_NUM*LINE_SIZE; i++){
                memory[i] = id; //Writes the process into memory for the amount needed
                counter++;
                if (counter >= mem_needed){
                    start = i+1;
                    return 0; //Exit with success
                }
            }
            start = i+1;
            return 0;
        }
        i++;
    }
    
    //If the program has reached here, allocation has failed. Determine what to do from here:
    if (mem_left >= mem_needed){
        return 1; //Call for defrag
    }
    else{
        return 2; //Call to move on
    }
}



int place_process_bf(char id, int mem_needed, bool contiguous) //Place a process in memory in the method of next-fit, with input for contiguous or not
{ 
    const int SIZE = LINE_NUM*LINE_SIZE;
    int i = 0, counter = 0; //For loop indices
    int it = i; //Saves a previous value of i (or the beginning of the scanned portion)
    int mem_left = 0; //Counts the total amount of memory open to determine if defrag will help
    bool size_check = false; //Marks if the memory is being checked for a large enough space, as opposed to trying to find a space

    //strategy for best fit, store the list of potential spaces and determine the smallest feasible one
    bool space_found = false;
    int smallest_space = 500;
    int start_loc = -1;


    vector< pair<int, int> > mem_locs;


    //Search through memory for an open spot:
    while ( (i < SIZE) || (memory[i] == '.') ){ //Goes all the way through memory, but will finish checking the open area it's on

        if (memory[i] == '.'){ //If spot is open, check if it's big enough; increment counter for each succesive '.'
            if (!size_check){ //If this is the beginning of a new open spot, save the position
                it = i;
            }
            size_check = true;
            counter++;
            mem_left++;
        }
        else{
            if(size_check){
                mem_locs.push_back(make_pair(counter, it));
            }
            size_check = false;
            counter = 0;
        }

        if(counter >= mem_needed ){
            //if they are equal: CORNER CASE: SKIP IT
            space_found = true;
            start_loc = it;
        }
        i++;
    }
    //add to the memory locations if it is at the end of the block
    if(size_check){
        mem_locs.push_back(make_pair(counter, it));
    }

    //set a default value for the smallest location
    if (space_found){
        smallest_space = mem_locs[0].first;
        start_loc = mem_locs[0].second;
    }

    //determine the smallest space
    unsigned int y = 0;
    for( y = 0; y < mem_locs.size(); y++){
        if(mem_locs[y].first >= mem_needed && mem_locs[y].first < smallest_space){
            smallest_space = mem_locs[y].first;
            start_loc = mem_locs[y].second;
        }
    }
    
    //If space was found, write the process to the block
    if (space_found){ //Success! Spot has been found!
        counter = 0;
        for (i = start_loc; i < LINE_NUM*LINE_SIZE; i++){
            memory[i] = id; //Writes the process into memory for the amount needed
            counter++;
            if (counter >= mem_needed){
                return 0; //Exit with success
            }
        }
        return 0;
    }
    else if (mem_left >= mem_needed){
        return 1; //Call for defrag
    }
    else{
        return 2; //Call to move on
    }
}





int place_process_wf(char id, int mem_needed, bool contiguous) //Place a process in memory in the method of next-fit, with input for contiguous or not
{ 
    const int SIZE = LINE_NUM*LINE_SIZE;
    int i = 0, counter = 0; //For loop indices
    int it = i; //Saves a previous value of i (or the beginning of the scanned portion)
    int mem_left = 0; //Counts the total amount of memory open to determine if defrag will help
    bool size_check = false; //Marks if the memory is being checked for a large enough space, as opposed to trying to find a space

    //strategy for best fit, store the list of potential spaces and determine the smallest feasible one
    bool space_found = false;
    int largest_space = 500;
    int start_loc = -1;


    vector< pair<int, int> > mem_locs;


    //Search through memory for an open spot:
    while ( (i < SIZE) || (memory[i] == '.') ){ //Goes all the way through memory, but will finish checking the open area it's on

        if (memory[i] == '.'){ //If spot is open, check if it's big enough; increment counter for each succesive '.'
            if (!size_check){ //If this is the beginning of a new open spot, save the position
                it = i;
            }
            size_check = true;
            counter++;
            mem_left++;
        }else{
            if(size_check){
                mem_locs.push_back(make_pair(counter, it));
            }
            size_check = false;
            counter = 0;
        }
        if(counter >= mem_needed ){
            //if they are equal: CORNER CASE: SKIP IT
            space_found = true;
            start_loc = it;
        }
        i++;
    }
    //add to the memory locations if it is at the end of the block
    if(size_check){
        mem_locs.push_back(make_pair(counter, it));
    }
    //set a default value for the smallest location
    if (space_found){
        largest_space = mem_locs[0].first;
        start_loc = mem_locs[0].second;
    }

    unsigned int y = 0;
    for( y = 0; y < mem_locs.size(); y++){
        if(mem_locs[y].first >= mem_needed && mem_locs[y].first > largest_space){
            largest_space = mem_locs[y].first;
            start_loc = mem_locs[y].second;
        }
    }
    
    //If space was found, write the process to the block
    if (space_found){ //Success! Spot has been found!
        counter = 0;
        for (i = start_loc; i < LINE_NUM*LINE_SIZE; i++){
            memory[i] = id; //Writes the process into memory for the amount needed
            counter++;
            if (counter >= mem_needed){
                return 0; //Exit with success
            }
        }
        return 0;
    }
    else if (mem_left >= mem_needed){
        return 1; //Call for defrag
    }
    else{
        return 2; //Call to move on
    }
}






int place_process_Non_Contig(char id, int mem_needed) //Place a process in memory in the method of next-fit, with input for contiguous or not
{ 
    const int SIZE = LINE_NUM*LINE_SIZE;
    int i = 0;
    int mem_left = 0; //Counts the total amount of memory open to determine if defrag will help


    //Search through memory for an open spot:
    while ( (i < SIZE) || (memory[i] == '.') ){

        if (memory[i] == '.'){
            mem_left++;
        }
        i++;
    }
    //insert into the block if large enough, don't otherwise
    if (mem_left >= mem_needed){
        int counter = 0;
        int c = 0;
        for (c = 0; c < LINE_NUM*LINE_SIZE; c++){
            if (memory[c] == '.' && counter < mem_needed){
                memory[c] = id;
                counter++;
            }
            if (counter >= mem_needed){
                return 0; //Exit with success
            }
        }
        return 0;
    }
    else{
        return 1;
    }
}







void fit_non_contiguous(vector<process>* processes){
    int t = 0; //time counter
    int delay = 0; //Value added to t when printed, since out-of-memory delays also delay everything else
    int temp_int = 0;
    unsigned int i = 0, j = 0, k = 0; //For loop indices
    bool done = false; //Keeps track of whether all processes have finished

    cout << "time " << t + delay << "ms: Simulator started (Non-contiguous)\n";
    
    for(t = 0; !done; t++)
    {
        //Loop for removal first
        for(i = 0; i < processes->size(); i++){
            for(j = 0; j < (*processes)[i].arrivals.size(); j++){
                if (t == (*processes)[i].arrivals[j].first + (*processes)[i].arrivals[j].second) //If the process has finished, remove it from memory
                {
                    bool did_run = false;
                    for (k = 0; k < LINE_NUM*LINE_SIZE; k++){
                        if (memory[k] == (*processes)[i].id){
                            memory[k] = '.';
                            did_run = true;
                        }
                    }
                    if(did_run){
                        printf("time %dms: Process %c removed:\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                }
            }
        }
        //loop for arrivals second
        for(i = 0; i < processes->size(); i++){
            for(j = 0; j < (*processes)[i].arrivals.size(); j++){
                if (t == (*processes)[i].arrivals[j].first) //If the process has arrived, add it to memory
                {
                    printf("time %dms: Process %c arrived (requires %d frames)\n", t + delay, (*processes)[i].id, (*processes)[i].mem_needed);
                    
                    temp_int = place_process_Non_Contig((*processes)[i].id, (*processes)[i].mem_needed); //Saves the return value to see what to do next
                    if (temp_int == 0){
                        printf("time %dms: Placed process %c:\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                    else{ //Insufficient memory, skip the process and move on
                        printf("time %dms: Cannot place process %c -- skipped!\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                }
            }
        }
        //Check if every process has completed
        done = true;
        for(i = 0; i < processes->size(); i++){
            for(j = 0; j < (*processes)[i].arrivals.size(); j++){
                if (t >= (*processes)[i].arrivals[j].first + (*processes)[i].arrivals[j].second){

                }
                else{
                    done = false;
                }
            }
        }
        if(done){
            break;
        }
    }
    printf("time %dms: Simulator ended (Non-contiguous)\n", t + delay);
}






void fit_algors(vector<process>* processes, int algor_num){
    int t = 0; //time counter
    int delay = 0; //Value added to t when printed, since out-of-memory delays also delay everything else
    int temp_int = 0;
    unsigned int i = 0, j = 0, k = 0; //For loop indices
    bool done = false; //Keeps track of whether all processes have finished
    
    if(algor_num == 0){
        cout << "time " << t + delay << "ms: Simulator started (Contiguous -- Next-Fit)\n";
    }
    else if(algor_num == 1){
        cout << "time " << t + delay << "ms: Simulator started (Contiguous -- Best-Fit)\n";
    }
    else if(algor_num == 2){
        cout << "time " << t + delay << "ms: Simulator started (Contiguous -- Worst-Fit)\n";
    }
    
    for(t = 0; !done; t++)
    {
        //Loop for removals first
        for(i = 0; i < processes->size(); i++){
            for(j = 0; j < (*processes)[i].arrivals.size(); j++){
                if (t == (*processes)[i].arrivals[j].first + (*processes)[i].arrivals[j].second) //If the process has finished, remove it from memory
                {
                    bool did_run = false;
                    for (k = 0; k < LINE_NUM*LINE_SIZE; k++){
                        if (memory[k] == (*processes)[i].id){
                            memory[k] = '.';
                            did_run = true;
                        }
                    }
                    if(did_run){
                        printf("time %dms: Process %c removed:\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                }
            }
        }
        //Loop for arrivals second
        for(i = 0; i < processes->size(); i++){
            for(j = 0; j < (*processes)[i].arrivals.size(); j++){
                if (t == (*processes)[i].arrivals[j].first) //If the process has arrived, add it to memory
                {
                    printf("time %dms: Process %c arrived (requires %d frames)\n", t + delay, (*processes)[i].id, (*processes)[i].mem_needed);
                    
                    if(algor_num == 0){
                        temp_int = place_process_nf((*processes)[i].id, (*processes)[i].mem_needed, true); //Saves the return value to see what to do next
                    }
                    else if(algor_num == 1){
                        temp_int = place_process_bf((*processes)[i].id, (*processes)[i].mem_needed, true);
                    }
                    else if(algor_num == 2){
                        temp_int = place_process_wf((*processes)[i].id, (*processes)[i].mem_needed, true);
                    }



                    if (temp_int == 0){
                        printf("time %dms: Placed process %c:\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                    else if (temp_int == 1){ //If fails to find a spot, but there is sufficient memory, defrag, then try again
                        printf("time %dms: Cannot place process %c -- starting defragmentation\n", t + delay, (*processes)[i].id);
                        temp_int = defrag();
                        delay += temp_int;
                        printf("time %dms: Defragmentation complete (moved %d frames: %c", t + delay, temp_int, v[0]);
                        for (k = 1; k < v.size(); k++){
                            printf(", %c", v[k]);
                        }
                        printf(")\n");
                        print_mem();



                        if(algor_num == 0){
                            temp_int = place_process_nf((*processes)[i].id, (*processes)[i].mem_needed, true); //Recursion not needed as it should work this time
                        }
                        else if(algor_num == 1){
                            temp_int = place_process_bf((*processes)[i].id, (*processes)[i].mem_needed, true); 
                        }
                        else if(algor_num == 2){
                            temp_int = place_process_wf((*processes)[i].id, (*processes)[i].mem_needed, true); 
                        }
                        


                        if (temp_int != 0){
                            perror("Error: Placement failed");
                            exit(EXIT_FAILURE);
                        }
                        printf("time %dms: Placed process %c:\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                    else{ //Insufficient memory, skip the process and move on
                        printf("time %dms: Cannot place process %c -- skipped!\n", t + delay, (*processes)[i].id);
                        print_mem();
                    }
                }
            }
        }
        //Check if every process has completed
        done = true;
        for(i = 0; i < processes->size(); i++){
            for(j = 0; j < (*processes)[i].arrivals.size(); j++){
                if (t >= (*processes)[i].arrivals[j].first + (*processes)[i].arrivals[j].second){
                    
                }
                else{
                    done = false;
                }
            }
        }
        if(done){
            break;
        }
    }
    if(algor_num == 0){
        printf("time %dms: Simulator ended (Contiguous -- Next-Fit)\n", t + delay);
    }
    else if(algor_num == 1){
        printf("time %dms: Simulator ended (Contiguous -- Best-Fit)\n", t + delay);
    }
    else if(algor_num == 2){
        printf("time %dms: Simulator ended (Contiguous -- Worst-Fit)\n", t + delay);
    }
    
}



void opt(vector<int>* pages){
    printf("Simulating OPT with fixed frame size of %d\n", FRAME_SIZE);
    int frame[FRAME_SIZE]; //Holds the pages currently loaded
    unsigned int future[FRAME_SIZE]; //Saves results of k for each page
    unsigned int i = 0, k = 0, max_k = 0; //max_k is the largest time until next use
    int j = 0, max_j = 0; //max_j is the largest time until next use
    int victim = 0; //the page that is removed
    int faults = 0;
    bool found = false; //Keeps track of whether a search is successful

    for(j = 0; j < FRAME_SIZE; j++){
        frame[j] = 0; //Initialize pages to zero, since the numbers are all 1-9
    }
    for (i = 0; i < pages->size(); i++){
    
        for(j = 0; j < FRAME_SIZE; j++){
            if((*pages)[i] == frame[j]){
                found = true;
                break;
            }
        }
        if (found){ //Page is in the frame; do nothing
            found = false;
            continue;
        }
        
        //If not, cause a page fault and add it
        max_k = 0;
        max_j = 0;
        for (j = 0; j < FRAME_SIZE; j++){
            if (frame[j] == 0){ //If the spot is open, take it and be done with it
                max_j = j;
                break;
            }
            for (k = i; k < pages->size(); k++){ //Otherwise, see how far in the future each process is used
            future[j] = k;
                if ((*pages)[k] == frame[j]){
                    break;
                }
            }

            if (k > max_k){ //Keep a run of the largest time in the future; if it beats the max, make that the new victim
                max_k = k;
                max_j = j;
            }
        }
        
        //Check for ties. If one is found, overwrite the index found above.
        found = false;
        if(i >= FRAME_SIZE){
            for (j = 0; j < FRAME_SIZE; j++){
                for (k = j+1; k < FRAME_SIZE; k++){
                    if (future[j] == future[k]){
                        /*A tie has been found. In actuality, a tie can only happen if neither process
                        is used again at all, since two processes cannot be in the same place in the vector.*/
                        found = true;
                        break;
                    }
                }
                if (found){
                    break;
                }
            }
            for (j = 1; j < FRAME_SIZE; j++){
                for (k = j+1; k < FRAME_SIZE; k++){
                    if (future[j] == future[k]){
                        /*A tie has been found. In actuality, a tie can only happen if neither process
                        is used again at all, since two processes cannot be in the same place in the vector.*/
                        found = true;
                        break;
                    }
                }
                if (found){
                    break;
                }
            }
        }

        
        if (found){ //Break the tie by choosing the lowest-numbered process.
            max_j = 0;
            int tiebreak_num_smallest = frame[j];
            for (j = 0; j < FRAME_SIZE; j++){
                if(frame[j] < tiebreak_num_smallest){
                    tiebreak_num_smallest = frame[j];
                    max_j = j;
                }
            }
        }
        
        victim = frame[max_j]; //Save the victim's number before it's changed
        frame[max_j] = (*pages)[i]; //Store the new page in whatever space was decided

        printf("referencing page %d [mem:", (*pages)[i]);
        for(j = 0; j < FRAME_SIZE; j++){
            if (frame[j] == 0){
                printf(" .");
            }
            else{
                printf(" %d", frame[j]);
            }
        }
        printf("] PAGE FAULT ");
        if (victim == 0){
            printf("(no victim page)\n");
        }
        else{
            printf("(victim page %d)\n", victim);
        }
        faults++;
    }
    printf("End of OPT simulation (%d page faults)\n", faults);
}


void lru(vector<int>* pages){
    printf("Simulating LRU with fixed frame size of %d\n", FRAME_SIZE);
    int frame[FRAME_SIZE]; //Frame for pages
    int recency[FRAME_SIZE]; //Count of how recently the page in the corresponding frame slot was used
    unsigned int i;
    int j;
    int faults = 0;
    bool foundPage; //Boolean set when we find a page already in the frame
    
    //initialize the frame
    for(i = 0; i < FRAME_SIZE; i++){ //Initializes values to zero, assuming there is no page zero
        frame[i] = 0;
    }
    //Run through the entirety of the page list
    for(i = 0; i < pages->size(); i++){
        foundPage = false;

        //Printout the frame
        for(j = 0; j < FRAME_SIZE; j++){ 
            if (frame[j] == (*pages)[i])
                foundPage = true;
        }
        //If the page was in the frame, just say ok and move on
        if (foundPage){
            for(j = 0; j < FRAME_SIZE; j++){
                if(frame[j] == (*pages)[i])
                    recency[j] = 0;
                recency[j] += 1;
            }
            continue;
        }
        
        //We have an int that serves to compare the various recencies of the pages, and 
        //an int that keeps track of which page has that recency.
        int lowestRecency = 0;
        int selectedPage;
        //int to see if there was a victim page after it's been replaced;
        int victim;
        
        //Check each slot in the frame for recency
        for(j = 0; j < FRAME_SIZE; j++){
            //If there's an empty slot, just take that one
            if(frame[j] == 0){
                victim = 0;
                selectedPage = j;
                break;
            }
            if(recency[j] > lowestRecency){
                lowestRecency = recency[j];
                selectedPage = j;
                victim = frame[j];
            }
        }
        
        //Replace the selected page and reset recency
        frame[selectedPage] = (*pages)[i];
        recency[selectedPage] = 0;
        
        printf("referencing page %d [mem:", (*pages)[i]);
        for (j = 0; j < FRAME_SIZE; j++){
            if (frame[j] != 0) //If the slot is full, print the page number
                printf(" %d", frame[j]);
            else //If empty, say so
                printf(" .");
        }
        printf("] PAGE FAULT ");
        if(victim > 0){
            printf("(victim page %d)\n", victim);
        }else{
            printf("(no victim page)\n");
        }
        //advance the recency counters
        for(j = 0; j < FRAME_SIZE; j++){
            recency[j] += 1;
        }
        faults++;
    }
    printf("End of LRU simulation (%d page faults)\n", faults);
}

void lfu(vector<int>* pages){
    printf("Simulating LFU with fixed frame size of %d\n", FRAME_SIZE);
    int frame[FRAME_SIZE]; //Frame for pages
    int frequency[FRAME_SIZE]; //Count of how frequently the page in the corresponding frame slot is used
    unsigned int i;
    int j;
    int faults = 0;
    bool foundPage; //Boolean set when we find a page already in the frame
    
    //initialize the frame
    for(i = 0; i < FRAME_SIZE; i++){ //Initializes values to zero, assuming there is no page zero
        frame[i] = 0;
    }
    //Run through the entirety of the page list
    for(i = 0; i < pages->size(); i++){
        foundPage = false;

        //Printout the frame
        for(j = 0; j < FRAME_SIZE; j++){ 
            if (frame[j] == (*pages)[i])
                foundPage = true;
        }
        //If the page was in the frame, just say ok and move on
        if (foundPage){
            for(j = 0; j < FRAME_SIZE; j++){
                if(frame[j] == (*pages)[i])
                    frequency[j] += 1;
            }
            continue;
        }
        
        //We have an int that serves to compare the various frequencies of the pages, and 
        //an int that keeps track of which page has that frequency.
        int lowestFrequency = pages->size();
        int selectedPage;
        //int to see if there was a victim page after it's been replaced;
        int victim;
        
        //Check each slot in the frame for recency
        for(j = 0; j < FRAME_SIZE; j++){
            //If there's an empty slot, just take that one
            if(frame[j] == 0){
                victim = 0;
                selectedPage = j;
                break;
            }
            if(frequency[j] < lowestFrequency){
                lowestFrequency = frequency[j];
                selectedPage = j;
                victim = frame[j];
            }else if(frequency[j] == lowestFrequency && frame[j] < victim){
                selectedPage = j;
                victim = frame[j];
            }
        }
        
        //Replace the selected page and reset recency
        frame[selectedPage] = (*pages)[i];
        frequency[selectedPage] = 1;
        
        printf("referencing page %d [mem:", (*pages)[i]);
        for (j = 0; j < FRAME_SIZE; j++){
            if (frame[j] != 0) //If the slot is full, print the page number
                printf(" %d", frame[j]);
            else //If empty, say so
                printf(" .");
        }
        printf("] PAGE FAULT ");
        if(victim > 0){
            printf("(victim page %d)\n", victim);
        }else{
            printf("(no victim page)\n");
        }
        faults++;
    }
    printf("End of LFU simulation (%d page faults)\n", faults);
}



//for reading the block memory thing file
vector<process> read_file(char* filename){
    //Reads and parses the given file, and returns the process vector
    int num_processes = 0;
    char* temp_s = (char*) calloc(10, sizeof(char));
    char temp_c;
    int arr_time = 0, run_time = 0; //Temp vars for adding to the vector
    int i = 0;
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
      fprintf(stderr, "Can't open input file\n");
      exit(EXIT_FAILURE); 
    }    
    fscanf(file, "%d", &num_processes); //First line is the number of processes
    vector<process> processes; // (num_processes, temp_proc); //Initalizes processes to the given number of blank processes
    fgetc(file); //Skip the \n
    while (temp_c != EOF){
        process temp_proc;
        temp_c = fgetc(file);
        temp_proc.id = temp_c;
        fscanf(file, "%s", temp_s); // Gets one full word
        temp_proc.mem_needed = atoi(temp_s);
        temp_c = fgetc(file);
        while(temp_c == ' '){
            //Gets the different arrivals, for which we don't know how many there will be
            fscanf(file, "%s", temp_s); 
            sscanf(temp_s, "%d/%d ", &arr_time, &run_time);
            temp_proc.arrivals.push_back(make_pair(arr_time, run_time));
            temp_c = fgetc(file);
        }
        processes.push_back(temp_proc);
        i++;
    }
    fclose (file);

    //sort the processes by alphabetical order to ensure that ties are broken correctly
    process temp;
    int stop = num_processes;
    for(int counter = num_processes-1; counter>=0; counter--)
    {
        for(int j=0; j<stop-1; j++)
        {
            if(processes[j].id>processes[j+1].id)
            {
                temp = processes[j+1];
                processes[j+1] = processes[j];
                processes[j]=temp;
            }
        }
        stop--;
    }
    return processes;
}


//for reading Virtual memory file
vector<int> page_file_read(char* filename){
    FILE* file = fopen(filename, "r");
    char* temp_c = (char*) malloc(2 * sizeof(char)); //Has to be string for atoi
    temp_c[0] = fgetc(file);
    temp_c[1] = '\0';
    vector<int> pages; //The order of the page references
    pages.push_back(atoi(temp_c));
    temp_c[0] = fgetc(file);
    while(temp_c[0] != EOF){ //File consists only of single digit numbers delimited by spaces, so reading is very simple
        temp_c[0] = fgetc(file);
        if(temp_c[0] == '\n'){
            break;
        }
        pages.push_back(atoi(temp_c));
        temp_c[0] = fgetc(file);
        if(temp_c[0] == '\n'){
            break;
        }
    }
    fclose(file);
    return pages;
}



int main(int argc, char* argv[]){
    int i = 0; // for loop indices
    //Initialize the memory to be empty:
    for (i = 0; i < LINE_NUM*LINE_SIZE; i++) { memory[i] = '.';}
    
    if (argc > 3){
        fprintf(stderr, "Too many arguments\n");
        exit(EXIT_FAILURE); 
    }
    else if (argc < 3){
        fprintf(stderr, "ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>\n");
        exit(EXIT_FAILURE); 
    }
	
	//Read the input file into a vector of processes
    vector<process> processes = read_file(argv[1]);
    
    fit_algors(&processes, 0);
    cout<<"\n";

    for (i = 0; i < LINE_NUM*LINE_SIZE; i++) { memory[i] = '.'; }
    fit_algors(&processes, 1);

    cout<<"\n";
    for (i = 0; i < LINE_NUM*LINE_SIZE; i++) { memory[i] = '.'; }
    fit_algors(&processes, 2);

    cout<<"\n";
    for (i = 0; i < LINE_NUM*LINE_SIZE; i++) { memory[i] = '.'; }
    fit_non_contiguous(&processes);


    //virtual memory
    vector<int> pages = page_file_read(argv[2]);
    cout<<"\n";
    opt(&pages);
    cout<<"\n";
    lru(&pages);
    cout<<"\n";
    lfu(&pages);

    return EXIT_SUCCESS;
}