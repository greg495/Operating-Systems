#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

char* sub_exp(char * expression, int i);
int fork_num(char* temp_str);
int parse_and_calc(char * expression);
int fork_expr(char* subexpr);
int find_operands(char * expression);

char* sub_exp(char * expression, int i) //Takes the full expression and outputs the subexpression starting at index i for forking
{
    int paren = 0; //for counting parentheses
    int j = i;
    char * temp_str = calloc(20, sizeof(char));
    strcpy(temp_str, ""); //Clears out temp_str
    char temp_c = (char) expression[i];
    while ( (!((temp_c == ')') && (paren == 0))) && (temp_c != '\0') )
    {
        temp_c = (char) expression[j];
        if( temp_c == '(' )
        {
            paren++;
        }
        else if( temp_c == ')' )
        {
            paren--;
        }
        /*Counts the number of open paren minus number of closed paren.
        A full expression only ends on a ')' and when the number of each type of paren are equal, to account for nesting.*/ 
        j++;
    }
    if (temp_c == '\0')
    {
        //Will also return error if the end of the string is reached before full subexpression found.
        printf("Parentheses mismatch; exiting\n");
        exit(EXIT_FAILURE);
    }
    
    //Get rid of of paren on each side:
    
    strncpy( temp_str, &(expression[i+1]), j-i-2 );
    temp_str[j-1] = '\0';//Manually re-adds null terminator
    /*This is where forking stuff happens*/
    return temp_str;
}


int fork_num(char* temp_str) //Forks if just a number
{
    pid_t pid;
    int operand = 0;
    int p[2];  /* array to hold the two pipe (file) descriptors:
                p[0] is the read end of the pipe
                p[1] is the write end of the pipe */
    int rc = pipe( p );
    
    if ( rc == -1 )
    {
        puts( "pipe() failed" );
        fflush(NULL);
        exit(EXIT_FAILURE);
    }
    pid = fork();
    fflush(NULL);
    if (pid == 0) //If in child
    {
        close( p[0] );  /* close the read end of the pipe */
        p[0] = -1;      /* this should avoid human error.... */
            /* write some data to the pipe */
        operand = atof(temp_str); //This is proof of the use of pipes; parent doesn't know this value unless passed
        printf("PID %d: My expression is \"%d\"\n", getpid(), operand);
        fflush(NULL);
        printf("PID %d: Sending \"%d\" on pipe to parent\n", getpid(), operand);
        fflush(NULL);
        write( p[1], &(operand), sizeof(int) );
        exit(EXIT_SUCCESS); //Terminates child once done
    }
    else //If in parent
    {
        close( p[1] );  /* close the write end of the pipe */
        p[1] = -1;      /* this should avoid human error.... */
        read( p[0], &(operand), sizeof(int) );   /* BLOCKING CALL */                
        return operand;
    }
}

int find_operands(char * expression) //Gives the number of operands in an expression
{
    int num_operands = 0;
    int paren = 0;
    int i = 0;
    for (i = 2; i <= (int) strlen(expression); i++) //Skips the inital operation
    {
        if ((((char) expression[i] == ' ') || ((char) expression[i] == '\0')) && ((char) expression[i-1] != ')'))//End of argument
        {
            if (paren == 0)
            {
                num_operands++;
            }
        }
        else if( (char) expression[i] == '(' )//Start of new parenthetical expression. Time to ignore operands within.
        {
            if (paren == 0) // If not in subexpression
            {
                num_operands++;
            }
            paren++;
        }
        else if ((char) expression[i] == ')')
        {
            if (paren == 0) // If not in subexpression
            {
                num_operands++;
            }
            paren--;
        }
    }
    return num_operands;
}

int parse_and_calc(char * expression) //Parses the expression and gives the operator and operands
{
    int i = 0; //for loop index
    char* temp_str = calloc(10, sizeof(char)); //Used for all sorts of stuff
    char temp_c; //Because C is silly, making an intermediate for (char) expression[i] is easier
    int operand_size = 8;
    int val = 0; //final answer for this fork
    const pid_t pid = getpid(); //Original Process ID; compared to see if in child or parent
    
    char* operation = calloc(10, sizeof(char)); //the operation for the expression; must be a string to capture invalid operations
    int* operands = (int*) calloc(operand_size, sizeof(int));
    int num_operands = find_operands(expression); //number of operands
    int ops_count = 0; //Keeps track of where to put numbers in operands
    strcpy(temp_str, ""); //Clears out temp_str
    sscanf (expression,"%s",operation);
    
    printf("PID %d: My expression is \"(%s)\"\n", pid, expression); //Expression lacks the parentheses, therefore I add them back in for printing
    fflush(NULL);
    if (((int) strlen(operation) > 1) || 
    (((char) operation[0] != '+') && ((char) operation[0] != '-') && ((char) operation[0] != '*') && ((char) operation[0] != '/'))) //If invalid operator
    {
        printf("PID %d: ERROR: unknown \"%s\" operator; exiting\n", pid, operation);
        exit(EXIT_FAILURE);
    }
    printf("PID %d: Starting \"%c\" operation\n", pid, operation[0]);
    fflush(NULL);
    //printf("There are %d operands\n", num_operands);
    fflush(NULL);
    for (i = 2; i <= (int) strlen(expression); i++) //Skips the inital operation
    {
        temp_c = (char) expression[i];

        if (((temp_c == ' ') || (temp_c == '\0')) && ((char) expression[i-1] != ')'))//End of argument
        {
            //Assume it's a numeric argument
            if (num_operands < 2) //Why is this here? Allows one fork, then checks if enough operands, to match Submitty.
            {
                printf("PID %d: ERROR: not enough operands; exiting\n", pid);
                fflush(NULL);
            }
            if (((char) operation[0] == '/') && (atoi(temp_str) == 0)) //Same here
            {
                printf("PID %d: ERROR: division by zero is not allowed; exiting\n", pid);
                fflush(NULL);
                exit(EXIT_FAILURE);
            }
            operands[ops_count] = fork_num(temp_str);
            if (num_operands < 2) //The error is thrown before the fork, but doesn't exit until after...ask Goldschmidt
            {
                exit(EXIT_FAILURE);
            }

            ops_count++;
            strcpy(temp_str, ""); //Clears out temp_str
            //printf("i is %d\n", i);
        }
        
        else if( temp_c == '(' )//Start of new parenthetical expression. Time to fork!
        {
            temp_str = sub_exp(expression, i);
            operands[ops_count] = fork_expr(temp_str); //Note the parent in fork_expr is original
            if (operands[ops_count] == 0)
            {
                printf("PID %d: ERROR: division by zero is not allowed; exiting\n", pid);
                exit(EXIT_FAILURE);
            }
            ops_count++;
            i += (int) strlen(temp_str)+2; //Skips the subexpression in original process so it's not read again
        }
        else //Continuation of previous argument
        {
            strncat(temp_str, &temp_c, 1);
        }

    }
    
    if (operation[0] == '+') //Addition
    {
        val = operands[0];
        for(i = 1; i < num_operands; i++)
        {
            val += operands[i];
        }
    }
    else if (operation[0] == '-')
    {
        val = operands[0];
        for(i = 1; i < num_operands; i++)
        {
            val -= operands[i];
        }
    }
    else if (operation[0] == '*')
    {
        val = operands[0];
        for(i = 1; i < num_operands; i++)
        {
            val *= operands[i];
        }
    }
    else if (operation[0] == '/')
    {
        val = operands[0];
        for(i = 1; i < num_operands; i++)
        {
            val /= operands[i];
        }
    }
    return val;
}

int fork_expr(char* subexpr) //Forks if a new subexpression
{
    pid_t pid;
    int operand = 0;
    int p[2];  /* array to hold the two pipe (file) descriptors:
                p[0] is the read end of the pipe
                p[1] is the write end of the pipe */
    int rc = pipe( p );
    
    if ( rc == -1 )
    {
        printf( "pipe() failed\n" );
        exit(EXIT_FAILURE);
    }
    pid = fork();
    fflush(NULL);
    if (pid == 0) //If in child
    {
        close( p[0] );  /* close the read end of the pipe */
        p[0] = -1;      /* this should avoid human error.... */
            /* write some data to the pipe */
        operand = parse_and_calc(subexpr);
        printf("PID %d: Processed \"(%s)\"; sending \"%d\" on pipe to parent\n", getpid(), subexpr, operand);
        fflush(NULL);
        write( p[1], &(operand), sizeof(int) );
        exit(EXIT_SUCCESS); //Terminates child once done
    }
    else //If in parent
    {
        close( p[1] );  /* close the write end of the pipe */
        p[1] = -1;      /* this should avoid human error.... */
        read( p[0], &(operand), sizeof(int) );   /* BLOCKING CALL */                
        return operand;
    }
}


int main(int argc, char *argv[]){
    FILE * file = fopen(argv[1], "r");
    int expression_size = 30;
    char * expression = (char*) calloc(expression_size, sizeof(char));
    char * temp_str; //Used for all sorts of stuff
    pid_t pid = getpid(); //Process ID
    
    if (file == NULL)
    {
        printf("File not found\n");
        return EXIT_FAILURE;
    }
    char temp_char = fgetc(file); //For iterating through file
    
    while (( temp_char != '(' ) && ( temp_char != EOF )) //Find the start of the expression, or end of the file, whichever comes first
    {
        temp_char = fgetc(file);
    }
    if (temp_char == EOF) //If no parentheses, there is no expression
    {
        printf("No expression found\n");
        return EXIT_FAILURE;
    }
    
    while ( (fgets (expression, 30, file)) != NULL ) //Read the expression into the variable. Purposely skips the '(' to help parsing.
    {
        if ( (bool) (expression[expression_size - 1]) ) //If the array is full, reallocate
        {
            expression_size *= 2;
            expression = (char*) realloc (expression, expression_size * sizeof(char));
        }
    }
    fclose(file);

    //All this to get rid of trailing ')':
    temp_str = (char*) calloc(expression_size, sizeof(char));
    strncpy( temp_str, expression, (int) strlen(expression)-2 );
    temp_str = strcat (temp_str, "\0"); //Manually re-adds null terminator
    expression = temp_str;
    if (pid) //Only put out final answer if in original process
    {
        printf("PID %d: Processed \"(%s)\"; final answer is \"%d\"\n", pid, expression, parse_and_calc(expression));
        fflush(NULL);
        free(expression);
        exit (EXIT_SUCCESS);
    }
    free(expression);
    exit (EXIT_FAILURE);
}