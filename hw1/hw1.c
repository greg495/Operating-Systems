/* Number of words that caused out-of-memory case: 39M words (Cloud 9). Freezes at the end for a bit for deallocation. 
   Cygwin only took 8000 words before seg faulting*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[]){
    //argv[1] is the name of the input file, argv[2] is the word to search for
    int array_size = 8; // The size of the array which will be reallocated
    int i = 0; // Current count of the number of words
    int j = 0; // number of characters in current word
    int k = 0; // Length of word
    //int repeats = 0; // Counts number of times to read through the file (used for breaking)
    size_t len;
    char** story = (char**) calloc(array_size, sizeof(char*)); //Initially allocates the array itself
    char* temp_str = (char*) calloc (20, sizeof(char)); // Sets inital buffer to 20 chars
    FILE * file;
    
    if (argc > 3){
        fprintf(stderr, "Too many arguments\n");
        return EXIT_FAILURE; 
    }
    else if (argc < 3){
        fprintf(stderr, "Too few arguments. Enter the input file and the word to search for.\n");
        return EXIT_FAILURE;
    }
    file = fopen(argv[1], "r");
    if (file == NULL) {
      fprintf(stderr, "Can't open input file\n");
      return EXIT_FAILURE;
    }
    
    //story = (char**) calloc(array_size, sizeof(char*)); 
    for (j = 0; j < array_size; j++)
    {
        story[j] = (char*) calloc(1, sizeof(char)); // Initially allocates each word
    }
    if ((bool)story & (bool)story[j-1]){
        printf( "Allocated initial array of 8 character pointers.\n");
    }
    else {
        fprintf(stderr, "Initial allocation failed.\n");
        return EXIT_FAILURE;
    }
    
    //for (repeats = 0; repeats < 220000; repeats++){ //Read the file this many times; used for breaking
        while ( (fscanf(file, "%s", temp_str)) != EOF)     //Gets the next word
        {
            if (i+1 == array_size) // If array is filled
            {
                array_size *= 2;
                story = (char**) realloc(story, array_size*sizeof(char*));
                printf("Re-allocated array of %d character pointers.\n", array_size);
            }
            len = strlen(temp_str);
            story[i] = (char*) realloc(story[i], len*sizeof(char));
            memcpy(story[i], temp_str, len*sizeof(char));
            i++;
        }
        rewind(file); // Moves file pointer back to the top of the file to read through again
    //}
    fclose (file);
    printf("All done (successfully read %d words).\n", i);
    /*for(j = 0; j < array_size; j++)
    {
        printf("%s ", story[j]);
    }*/
    
    printf("Words containing substring \"%s\" are:\n", argv[2]);
    for(j = 0; j < i; j++)
    {
        if (strstr(story[j], argv[2]))
        {
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
    
    //Deallocate:
    
    for (j = 0; j < array_size; j++)
    {
        free(story[j]);
    }
    free(story);
    free(temp_str);

    return EXIT_SUCCESS;
}