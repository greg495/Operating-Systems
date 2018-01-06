#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fts.h>
#include <pthread.h>

/* global mutex variable and others*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    /*****************/
int num_words = 0; // Current count of the number of words; i in hw1
int array_size = 8; // The size of the array which will be reallocated
char** story; // Contains the words from all stories. Can't apparently be initialized here; is initialized in main.
    
typedef struct // thread_info //Struct that is passed into each thread
{
    char* filename; //The file name for this thread
    char* path; //The file path
    //char** story;
    pthread_t * tid;
} thread_t;

void * file_read(void * arg)
{
    puts("Entered thread function");
    //thread_t * thread = (thread_t *)arg;
    char* path = (char*) arg;
    //char** story = thread->story;
    //pthread_t mytid = pthread_self();
    //pthread_detach( mytid );

    size_t len;
    char* temp_str = (char*) calloc (20, sizeof(char)); // Sets inital buffer to 20 chars
    FILE * file;
    
    //pthread_t mytid = pthread_self();
    printf("MAIN THREAD: Assigned \"%s\" to child thread %ld.\n", path, (long int)pthread_self());
    file = fopen(path, "r");
    printf("Path is %s\n", path);
    if (file == NULL) {
        perror("Can't open a file");
        exit(EXIT_FAILURE);
    }
    
    while ( (fscanf(file, "%s", temp_str)) != EOF)     //Gets the next word
    {
        printf("temp_str is %s\n", temp_str);
        pthread_mutex_lock( &mutex );     /******************************/
        int i = num_words; //Locks and saves the index until writing is done, but allows other threads to choose other indices
        num_words++; //Increments global variable immediately for other threads
        if (i + 1 == num_words) // If array is filled, keep it locked and reallocate
        {
            array_size *= 2;
            story = (char**) realloc(story, array_size*sizeof(char*));
            printf("THREAD %ld: Re-allocated array of %d character pointers.\n", (long int) pthread_self(), array_size);
        }
        pthread_mutex_unlock( &mutex ); //Unlocks so that writing can occur simultaneously
        len = strlen(temp_str);
        story[i] = (char*) realloc(story[i], len*sizeof(char));
        strcpy(story[i], temp_str);
        printf("THREAD %ld: Added \"%s\" at index %d.\n", (long int) pthread_self(), temp_str, i);
    }
    //rewind(file); // Moves file pointer back to the top of the file to read through again
    fclose (file);
    
    /* dynamically allocate space to hold a return value */
    unsigned int * x = (unsigned int *)malloc( sizeof( unsigned int ) );

    /* return value is simply the thread ID */
    *x = pthread_self();
    pthread_exit( x );   /* terminate the thread, returning x to a waiting pthread_join() call */
    free( arg );
    return NULL;
}

static int ptree(char * const argv[], pthread_t* tid, char** story){
    puts("Entered ptree");
    FTS *ftsp;
    FTSENT *p, *chp;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    int i = 0, j = 0;
    int rc = 1;
    char file_ext[5]; // The extension telling the name of the file
    if ((ftsp = fts_open(argv, fts_options, NULL)) == NULL) {
        warn("fts_open");
        return -1;
    }
    /* Initialize ftsp with as many argv[] parts as possible. */
    chp = fts_children(ftsp, 0);
    if (chp == NULL) {
        perror("No files found");
        return 0;               /* no files to traverse */
    }
    while ((p = fts_read(ftsp)) != NULL) {
        if (p->fts_info == FTS_F) {
            //printf("f %s\n", p->fts_path);
            //pthread_t tid;   /* very similar to: pid_t pid; */
            //Takes the file extension to check file type:
            for (i = 0; i < 4; i++){ 
                file_ext[i] = (char) (p->fts_name[(int)strlen(p->fts_name) - 4 + i]);
            }
            file_ext[4] = '\0';
            //printf("File name is %s\n", p->fts_name);
            //printf("File extension is %s\n", file_ext);
            
            if (strcmp(file_ext, ".txt") == 0) //If it's a text file
            {
                printf("Path is %s\n", p->fts_path);
                thread_t * thread = (thread_t *) malloc(sizeof(thread_t)); //Stores info for this new thread
                thread->filename = (char*) calloc (20, sizeof(char)); 
                thread->path = (char*) calloc (20, sizeof(char)); 
                strcpy(thread->filename, p->fts_name);
                strcpy(thread->path, p->fts_path);
                //thread->size_ptr = size_ptr;
                //thread->words_ptr = i;
                printf("About to create thread\n");
                rc = pthread_create( &tid[j], NULL, file_read, (void*)(p->fts_path)); //create child thread
                if ( rc != 0 ){
                    fprintf( stderr, "pthread_create() failed (%d): %s\n", rc, strerror( rc ) );
                    exit(EXIT_FAILURE);
                }
                j++;
            }
        }
    }
    fts_close(ftsp);
    return 0;
}

int main(int argc, char *argv[]){
    //argv[1] is the name of directory containing the input files, argv[2] is the word to search for
    story = (char**) calloc(array_size, sizeof(char*)); //Initially allocates the array itself
    int num_files = 0;
    int i = 0; // for loop index
    int k = 0; // Length of word

    if (argc > 3){
        fprintf(stderr, "Too many arguments\n");
        return EXIT_FAILURE; 
    }
    else if (argc < 3){
        fprintf(stderr, "Too few arguments. Enter the input directory and the word to search for.\n");
        return EXIT_FAILURE;
    }
    

    //story = (char**) calloc(array_size, sizeof(char*)); 
    for (i = 0; i < array_size; i++)
    {
        story[i] = (char*) calloc(1, sizeof(char)); // Initially allocates each word
    }
    if ((bool)story & (bool)story[i-1]){
        printf( "Allocated initial array of 8 character pointers.\n");
    }
    else {
        fprintf(stderr, "Initial allocation failed.\n");
        return EXIT_FAILURE;
    }


    
    FTS *ftsp;
    FTSENT *p;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;

    if ((ftsp = fts_open(argv, fts_options, NULL)) == NULL) {
        warn("fts_open");
        return -1;
    }
    //Count the number of files, so you know how many threads will be joining:
    while ((p = fts_read(ftsp)) != NULL) { 
        if (p->fts_info == FTS_F) {
            //Takes the file extension to check file type:
            char file_ext[5];
            for (i = 0; i < 4; i++){ 
                file_ext[i] = (char) (p->fts_name[(int)strlen(p->fts_name) - 4 + i]);
            }
            file_ext[4] = '\0';
            if (strcmp(file_ext, ".txt") == 0) //If it's a text file
            {
                num_files++;
            }
        }
    }
    printf("Found %d file(s)\n", num_files);
    pthread_t* tid = (pthread_t*) calloc(num_files, sizeof(pthread_t));   /* keep track of the thread IDs */
    
    //Call the thread making function:
    if (ptree(argv + 1, tid, story) != 0){
        perror("File traversal error");
        return EXIT_FAILURE;
    }
    
    //Waits for all threads to terminate:
    puts("Joining for loop reached");
    for(i = 0; i < num_files; i++){
        //unsigned int * x;
        pthread_join( tid[i], NULL );    // BLOCKING CALL 
        //&x is the return value from pthread_exit() 
        /*printf( "MAIN THREAD: Joined a child thread that returned %u.\n", *x );
        free( x );*/
    }
    printf("MAIN THREAD: All done (successfully read %d words).\n", num_words);
    /*for(j = 0; j < array_size; j++)
    {
        printf("%s ", story[j]);
    }*/
    /*
    printf("MAIN THREAD: Words containing \"%s\" are:\n", argv[2]);
    for(j = 0; j < i; j++)
    {
        if (strstr(story[j], argv[2]))
        {
            printf("MAIN THREAD: ");
            for (k = 0; k < strlen(story[j]); k++){
                if ((story[j][k] != ',') & (story[j][k] != '.') & (story[j][k] != '!') & 
                (story[j][k] != '?') & (story[j][k] != ':') & (story[j][k] != ';') & (story[j][k] != '"') & (story[j][k] != '\n'))
                {
                    printf("%c", story[j][k]);
                }
            }
            printf("\n");
        }
    }
    */
    //Deallocate:
    
    for (i = 0; i < array_size; i++)
    {
        free(story[i]);
    }
    free(story);

    return EXIT_SUCCESS;
}