#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

// gcc -o parsePuzzles parseScrapedPuzzles.c buf.c -ljansson

#include "buf.h"
#include "jansson.h"

typedef struct descriptionNode descriptionNode ;
typedef struct nfa nfa ;

/*
Descriptions are implemented as a linked list. To allow for constant time appending, each node
tracks the tail, as well as the length. Each field:
    
    val --> the integer value of the description
    next --> the next description element
    tail --> the final description element (NULL if not set)
    length --> the length of the description
*/
struct descriptionNode{
    int val ;
    descriptionNode * next ;
    descriptionNode * tail ;
    int length ;
} ;

/*
The nfa struct is used to implement the automata version of the regular expression that 
encodes the description. Due to the unique structure of the automata (Σ = {0,1}), the
only possible transition arrows are from a zero or a one. Additionally, there will only
ever be a transition from the previous state (s_{i} to s_{i+1}) so you can capture all
of the necessary information to create the CNF formula with the three binary vectors
inOnes,inZeros, and selfZeros. Each field:

    states --> the number of states in the automata
    inOnes --> inOnes[i] == 1 iff state i of the automata has an incoming one transition
    inZeros --> inZeros[i] == 1 iff state i of the automata has an incoming zero transition
    selfZeros --> selfZeros[i] == 1 iff state i of the automata has an incoming zero
                    transition and that transition originates from state i (and not i-1)
*/
struct nfa {
    int states ;
    int * inOnes ;
    int * inZeros ;
    int * selfZeros ;
} ;

/*
appendDescription: descriptionNode * x int -> void
appendDescription([r_1,r_2,...,r_i],r) = [r_1,r_2,...,r_i,r]
*/
void appendDescription(descriptionNode * d, int runLength) ;
void printDescription(descriptionNode * d) ;

/*
buildNFA: descriptionNode * -> nfa *
buildNFA(d) = n, the automata representing d (as defined in section 2.3). Strings will be accepted by
n if they fit the description d (for any size line).
*/
nfa * buildNFA(descriptionNode * d) ;

void printNFA(nfa * n) ;

/*
stateCount: descriptionNode * -> int
stateCount(d) = s, the number of states in the automata representing d (as defined in section 2.3)
*/
int stateCount(descriptionNode * d) ;

/*
uniqueVarCount: descriptionNode * -> int
uniqueVarCount(d) = v, the number of distinct variables in the CNF formula encoding the 
description d (as defined in section 2.3)
*/
int uniqueVarCount(descriptionNode * d, int lineLength) ;

/*
clauseCount: descriptionNode * -> int
clauseCount(d) = c, the number of clauses in the CNF formula encoding the description d (as defined in section 2.3)
*/
int clauseCount(descriptionNode * d, int lineLength) ;

/*
buildConstraint: nfa * x int x int x descriptionNode * -> Buf
buildConstraint(n,stringVariables,variableIndex,d) = Ψ, where Ψ is satisfiable when there is a string input
to n that can be accepted. 

The parameter stringVariables is an array of the variables that correspond to the cells in the board that d 
is constraining (row 0 in an NxN board would be [x_0,...,x_{N-1}]). The parameter variableIndex is the minimum
 index of a fresh variable in Ψ.
*/
Buf buildConstraint(nfa * n, int * stringVars, int * varIndex, descriptionNode * d, int lineLength) ;

/*
digits: int -> int
digits(i) = d, the number of digits in the base-10 representation of i
*/
int digits(int number) ;

/*
formulaVarCount: descriptionNode * -> int
formulaVarCount(d) = v, the total number of variables in the CNF formula encoding the 
description d (as defined in section 2.3)
*/
int formulaVarCount(descriptionNode * d, int lineLength) ;
Buf emptyLine(int * stringVars, int lineLength) ;

void freeDescription(descriptionNode * d) ;

int main(void){
    int cnfGenerated = 0 ;
    // I parsed them in batches, so I had to increment this based on puzzle index
    for (int i = 35700 ; i > 0 ; i--){ 
        if (i % 500 == 0){
            printf("Iteration: %d\n", i) ;
        }

        /*
        There are gaps in the puzzle indices (by the website, not the scraping), but they are not regular. To account for this,
        I start at the max index and try to open each one, steadily decrementing. If the file pointer is NULL after trying to
        open the file, there is no puzzle with that index so the iteration of the loop is cut short and no parsing is attempted.
        */
        FILE * test ;
        char index[110] ;
        sprintf(index,"/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/puzzleJSONdir/%d.json", i) ;
        //sprintf(index,"/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/debuggingNonSquare/testBoard.json") ;
        test = fopen(index,"r") ;
        if (test != NULL){
            fclose(test) ; // Have to close the original file used to check for existence
            cnfGenerated += 1 ;
            json_t * json ;
            json_error_t error ;
            json = json_load_file(index,0,&error) ; // Load the JSON file

            int rowCount = json_integer_value(json_object_get(json,"rowCount")) ; // Determine the number of rows (it's no longer just N)
            descriptionNode ** rowDescriptions = malloc(rowCount * sizeof(descriptionNode)) ; // Construct the row description array, to be filled below

            json_t * rows ;
            rows = json_object_get(json,"rows") ; // An array of descriptions (each of which is an array of integers)
            for (int i = 0 ; i < rowCount ; i++){
                json_t * data ;
                data = json_array_get(rows,i) ; // One description (array of integers)
                if (json_array_size(data) == 0){ // If a row is empty
                    descriptionNode * empty = malloc(sizeof(descriptionNode)) ;
                    empty->length = 0 ;
                    rowDescriptions[i] = empty ;
                } else { // For all non-empty rows
                    descriptionNode * head = malloc(sizeof(descriptionNode)) ;
                    head->tail = NULL ;
                    head->length = 0 ;
                    for (int j = 0 ; j < json_array_size(data) ; j++){
                        appendDescription(head,json_integer_value(json_array_get(data,j))) ;
                    }
                    rowDescriptions[i] = head ;
                }
            }

            int columnCount = json_integer_value(json_object_get(json,"columnCount")) ; // Determine the number of columns (it's no longer just N)
            descriptionNode ** columnDescriptions = malloc(columnCount * sizeof(descriptionNode)) ; // Construct the column description array, to be filled below

            json_t * columns ;
            columns = json_object_get(json,"columns") ; // An array of descriptions (each of which is an array of integers)
            for (int i = 0 ; i < columnCount ; i++){
                json_t * data ;
                data = json_array_get(columns,i) ; // One description (array of integers)
                if (json_array_size(data) == 0){ // If a column is empty
                    descriptionNode * empty = malloc(sizeof(descriptionNode)) ;
                    empty->length = 0 ;
                    columnDescriptions[i] = empty ;
                } else { // For all non-empty columns
                    descriptionNode * head = malloc(sizeof(descriptionNode)) ;
                    head->tail = NULL ;
                    head->length = 0 ;
                    for (int j = 0 ; j < json_array_size(data) ; j++){
                        appendDescription(head,json_integer_value(json_array_get(data,j))) ;
                    }
                    columnDescriptions[i] = head ;
                }
            }

            int rowVars = 0 ; 
            int rowClauses = 0 ;

            for (int i = 0 ; i < rowCount ; i++){ // For each row, count the unique variables and clauses that will occur
                if (rowDescriptions[i]->length != 0){
                    rowVars += uniqueVarCount(rowDescriptions[i],columnCount) ;
                    rowClauses += clauseCount(rowDescriptions[i],columnCount) ;
                } else {
                    rowClauses += columnCount ; // You have a singleton clause for each cell in the row (the number of columns)
                }  
            }
            int columnVars = 0 ; 
            int columnClauses = 0 ;

            for (int i = 0 ; i < columnCount ; i++){ // For each column, count the unique variables and clauses that will occur
                if (columnDescriptions[i]->length != 0){
                    columnVars += uniqueVarCount(columnDescriptions[i],rowCount) ;
                    columnClauses += clauseCount(columnDescriptions[i],rowCount) ;
                } else {
                    columnClauses += rowCount ; // You have a singleton clause for each cell in the column (the number of rows)
                }
            }

            FILE * fp ;
            char index[50] ;
            sprintf(index,"ScrapedCNF/%d.cnf",i) ;
            //sprintf(index,"../debuggingNonSquare/testCNF.cnf") ;
            fp = fopen(index,"w") ;
            fprintf(fp,"p cnf %d %d\n",columnCount*rowCount + rowVars + columnVars,rowClauses + columnClauses) ;

            int * varIndex = malloc(sizeof(int)) ;
            *varIndex = columnCount*rowCount+1 ;

            for (int i = 0 ; i < rowCount ; i++){ // for each row...
                int stringVars[columnCount] ; // The number of string variables is the cells in the row (which is the number of columns)
                for (int j = 0 ; j < columnCount ; j++){
                    stringVars[j] = i*columnCount + j + 1 ;
                }
                if (rowDescriptions[i]->length != 0){
                    nfa * n = buildNFA(rowDescriptions[i]) ; // Build the NFA (see section 2.3 of thesis)
                    //printNFA(n) ;
                    Buf constraint = buildConstraint(n,stringVars,varIndex,rowDescriptions[i], columnCount) ; // Build the CNF formula (see section 2.3 of thesis)
                    fprintf(fp,"%s",buf_data(constraint)) ; // Dump the buffer to file

                    // clean up after yourself...
                    free(n->inOnes) ;
                    free(n->inZeros) ;
                    free(n->selfZeros) ;
                    free(n) ;
                    free(constraint) ;
                } else {
                    Buf constraint = emptyLine(stringVars, columnCount) ;
                    fprintf(fp,"%s",buf_data(constraint)) ;
                    free(constraint) ;
                }  
            }
            for (int i = 0 ; i < columnCount ; i++){ // for each column...
                int stringVars[rowCount] ; // The number of string variables is the cells in the column (which is the number of rows)
                for (int j = 0 ; j < rowCount ; j++){
                    stringVars[j] = j*columnCount + i + 1 ;
                }
                if (columnDescriptions[i]->length != 0){
                    nfa * n = buildNFA(columnDescriptions[i]) ; // Build the NFA (see section 2.3 of thesis)
                    
                    Buf constraint = buildConstraint(n,stringVars,varIndex,columnDescriptions[i],rowCount) ; // Build the CNF formula (see section 2.3 of thesis)
                    fprintf(fp,"%s",buf_data(constraint)) ; // Dump the buffer to file

                    // clean up after yourself...
                    free(n->inOnes) ;
                    free(n->inZeros) ;
                    free(n->selfZeros) ;
                    free(n) ;
                    free(constraint) ;
                } else {
                    Buf constraint = emptyLine(stringVars,rowCount) ;
                    fprintf(fp,"%s",buf_data(constraint)) ;
                    free(constraint) ;
                }  
            }
            // Clean Up Time!
            for (int i = 0 ; i < rowCount ; i++){
                freeDescription(rowDescriptions[i]) ;
                
            }
            for (int i = 0 ; i < columnCount ; i++){
                freeDescription(columnDescriptions[i]) ;
            }
            
            free(rowDescriptions) ;
            free(columnDescriptions) ;
            fclose(fp) ;
        } else {
            fclose(test) ;
        }
        
    }
    return 0 ;
}

/*
The documentation for these functions can be found in the file regExEncoding.c in 
the directory encoding of the GitHub repo. Sorry for being sloppy and not putting
these functions in a module.
*/


void appendDescription(descriptionNode * d,int runLength){
    if (d->tail == NULL){
        d->val = runLength ;
        d->tail = d ;
    } else {
        descriptionNode * newRun = malloc(sizeof(descriptionNode)) ;
        newRun->val = runLength ;
        d->tail->next = newRun ;
        d->tail = d->tail->next ;
    }
    d->length += 1 ;
    return ;
}

void printDescription(descriptionNode * d){
    if (d->tail == NULL){
        return ;
    }
    descriptionNode * temp = d ;
    printf("< ") ;
    while (temp != NULL){
        printf("%d ",temp->val) ;
        temp = temp->next ;
    }
    printf(">\n") ;
    return ;
}

int uniqueVarCount(descriptionNode * d, int lineLength){
    int t = 0 ; 
    int s = 0 ;
    descriptionNode * temp = d ;
    while (temp != NULL){
        s += temp->val ;
        t += 1 ;
        temp = temp->next ;
    }
    return (2*lineLength+1)*(t+s) + lineLength ;
}

int clauseCount(descriptionNode * d, int lineLength){
    int t = 0 ; 
    int s = 0 ;
    descriptionNode * temp = d ;
    while (temp != NULL){
        s += temp->val ;
        t += 1 ;
        temp = temp->next ;
    }
    return (5*lineLength+2)*(t+1+s) - 4 ;
}

Buf buildConstraint(nfa * n, int * stringVars, int * varIndex, descriptionNode * d, int lineLength){
    int stateVars[(lineLength+1) * n->states] ;
    for (int i = 0 ; i < (lineLength+1)*n->states ; i++){
        stateVars[i] = *varIndex ;
        *varIndex = *varIndex + 1 ;
    }

    int transitionVars[2 * lineLength * n->states] ;

    for (int i = 0 ; i < n->states ; i++){ // Fill in for k = 1 (the first, which is zero indexed)
        if (n->inZeros[i] == 1 || n->selfZeros[i] == 1){
            transitionVars[i] = *varIndex ;
            *varIndex = *varIndex + 1 ;
        } else {
            transitionVars[i] = 0 ;
        }
        if (n->inOnes[i] == 1){
            transitionVars[n->states + i] = *varIndex ;
            *varIndex = *varIndex + 1 ;
        } else {
            transitionVars[n->states + i] = 0 ;
        }
    }

    for (int k = 1 ; k < lineLength ; k++){ // You can then copy over from k=1, incrementing the variable counter each time
        for (int i = 0 ; i < n->states ; i++){
            if (transitionVars[i] != 0){
                transitionVars[2*k*n->states + i] = *varIndex ;
                *varIndex = *varIndex + 1 ;
            } else {
                transitionVars[2*k*n->states + i] = 0 ;
            }
            if (transitionVars[n->states + i] != 0){
                transitionVars[2*k*n->states + n->states + i] = *varIndex ;
                *varIndex = *varIndex + 1 ;
            } else {
                transitionVars[2*k*n->states + n->states + i] = 0 ;
            }
        }
    }

    // Now let's print out each to see if it's being developed properly
    /*
    printf("String Variables:\n") ;
    for (int i = 0 ; i < N ; i++){
        printf("%d ",stringVars[i]) ;
    }
    printf("\nState Variables:\n") ;
    for (int i = 0 ; i < (N+1)*n->states ; i++){
        printf("%d ",stateVars[i]) ;
    }
    printf("\nTransition Variables:\n") ;
    for (int i = 0 ; i < 2*N*n->states ; i++){
        printf("%d ",transitionVars[i]) ;
    }
    printf("\n") ; */

    // Set Up the Buffer
    int bufferSize = 4*clauseCount(d, lineLength) + (digits(*varIndex + uniqueVarCount(d,lineLength)) + 2)*formulaVarCount(d, lineLength) ;
    Buf dimacs = buf_new(bufferSize) ;


    for (int k = 0 ; k < lineLength ; k++){
        // First Constraint
        
        for (int i = 0 ; i < n->states ; i++){
            if (transitionVars[2*k*n->states + i] != 0){
                //printf("(-%d -%d) (-%d %d) ",transitionVars[2*k*n->states + i],stringVars[k],transitionVars[2*k*n->states + i],stateVars[(k+1)*n->states + i]) ;
                buf_append(dimacs,"-%d -%d 0\n-%d %d 0\n",transitionVars[2*k*n->states + i],stringVars[k],transitionVars[2*k*n->states + i],stateVars[(k+1)*n->states + i]) ;
            }
            if (transitionVars[(2*k+1)*n->states + i] != 0){
                //printf("(-%d %d) (-%d %d) ",transitionVars[(2*k+1)*n->states + i],stringVars[k],transitionVars[(2*k+1)*n->states + i],stateVars[(k+1)*n->states + i]) ;
                buf_append(dimacs,"-%d %d 0\n-%d %d 0\n",transitionVars[(2*k+1)*n->states + i],stringVars[k],transitionVars[(2*k+1)*n->states + i],stateVars[(k+1)*n->states + i]) ;
            }
        }
        //printf("\n") ;
        // Second Constraint
        for (int i = 0 ; i < n->states ; i++){
            //printf("(-%d",stateVars[k*n->states + i]) ;
            buf_append(dimacs,"-%d",stateVars[k*n->states + i]) ;
            if (n->selfZeros[i] != 0){ // zero self-loop transition
                buf_append(dimacs," %d",transitionVars[2*k*n->states + i]) ;
            }
            if (i != n->states - 1){
                if (n->inOnes[i+1] != 0){
                    buf_append(dimacs," %d",transitionVars[(2*k+1)*n->states + i + 1]) ;
                }
                if (n->inZeros[i+1] != 0){
                    buf_append(dimacs," %d",transitionVars[2*k*n->states + i + 1]) ;
                }
            }
            /*
            if (transitionVars[2*k*n->states + i] != 0){
                //printf(" %d",transitionVars[2*k*n->states + i]) ;
                buf_append(dimacs," %d",transitionVars[2*k*n->states + i]) ;
            }
            if (i != n->states - 1 && transitionVars[(2*k+1)*n->states + i+1] != 0){
                //printf(" %d",transitionVars[(2*k+1)*n->states + i+1]) ;
                buf_append(dimacs," %d",transitionVars[(2*k+1)*n->states + i+1]) ;
            } */
            //printf(") ") ;
            buf_append(dimacs," 0\n") ;
        }
        //printf("\n") ;
        // Third Constraint
        for (int i = 0 ; i < n->states ; i++){
            //printf("(-%d",stateVars[(k+1)*n->states + i]) ;
            buf_append(dimacs,"-%d",stateVars[(k+1)*n->states + i]) ;
            if (transitionVars[2*k*n->states + i] != 0){
                //printf(" %d",transitionVars[2*k*n->states + i]) ;
                buf_append(dimacs," %d",transitionVars[2*k*n->states + i]) ;
            }
            if (transitionVars[(2*k+1)*n->states + i] != 0){
                //printf(" %d",transitionVars[(2*k+1)*n->states + i]) ;
                buf_append(dimacs," %d",transitionVars[(2*k+1)*n->states + i]) ;
            }
            //printf(")") ;
            buf_append(dimacs," 0\n") ;
        }
        
        // Fourth Constraint
        //printf("(%d",stringVars[k]) ;
        buf_append(dimacs,"%d",stringVars[k]) ;
        for (int i = 0 ; i < n->states ; i++){
            if (transitionVars[2*k*n->states + i] != 0){
                //printf(" %d",transitionVars[2*k*n->states + i]) ;
                buf_append(dimacs," %d",transitionVars[2*k*n->states + i]) ;
            }
        }
        //printf(") ") ;
        buf_append(dimacs," 0\n") ;
        //printf("(-%d",stringVars[k]) ;
        buf_append(dimacs,"-%d",stringVars[k]) ;
        for (int i = 0 ; i < n->states ; i++){
            if (transitionVars[(2*k+1)*n->states + i] != 0){
                //printf(" %d",transitionVars[(2*k+1)*n->states + i]) ;
                buf_append(dimacs," %d",transitionVars[(2*k+1)*n->states + i]) ;
            } 
        }
        //printf(")\n") ;
        buf_append(dimacs," 0\n") ;
        //printf("\n") ;
        
        // Fifth Constraint
        for (int i = 0 ; i < n->states ; i++){
            if (transitionVars[2*k*n->states + i] != 0){
                //printf("(-%d",transitionVars[2*k*n->states + i]) ;
                buf_append(dimacs,"-%d",transitionVars[2*k*n->states + i]) ;
                if (n->inZeros[i] != 0){
                    //printf(" %d",stateVars[k*n->states + i - 1]) ;
                    buf_append(dimacs," %d",stateVars[k*n->states + i - 1]) ;
                }
                if (n->selfZeros[i] != 0){
                    //printf(" %d",stateVars[k*n->states + i]) ;
                    buf_append(dimacs," %d",stateVars[k*n->states + i]) ;
                }
                //printf(") ") ;
                buf_append(dimacs," 0\n") ;
            }
            if (transitionVars[(2*k+1)*n->states + i] != 0){
                //printf("(-%d %d)",transitionVars[(2*k+1)*n->states + i],stateVars[k*n->states + i - 1]) ;
                buf_append(dimacs, "-%d %d 0\n",transitionVars[(2*k+1)*n->states + i],stateVars[k*n->states + i - 1]) ;
            }
            
        }
        //printf("\n") ;
    }
    
    for (int i = 1 ; i < n->states ; i++){
        //printf("(-%d) ",stateVars[i]) ;
        buf_append(dimacs, "-%d 0\n",stateVars[i]) ;
    }
    //printf("\n") ;
    for (int i = n->states * lineLength; i < n->states * (lineLength+1) - 1 ; i++){
        //printf("(-%d) ",stateVars[i]) ;
        buf_append(dimacs, "-%d 0\n",stateVars[i]) ;
    }
    //printf("\n\n") ;
    //printf("BUFFER:\n%s",buf_data(dimacs)) ;
    return dimacs;
    
}

int digits(int number){
    int copy = number ;
    return (int) log10(copy) + 1 ;
}

int formulaVarCount(descriptionNode * d, int lineLength){
    int t = 0 ; 
    int s = 0 ;
    descriptionNode * temp = d ;
    while (temp != NULL){
        s += temp->val ;
        t += 1 ;
        temp = temp->next ;
    }
    return (14*lineLength+2)*t+8*lineLength-2+(11*lineLength+2)*s ;
}

Buf emptyLine(int * stringVars, int lineLength){
    Buf dimacs = buf_new(lineLength*(digits(stringVars[lineLength-1]) + 5)) ;
    for (int i = 0 ; i < lineLength ; i++){
        buf_append(dimacs,"-%d 0\n",stringVars[i]) ;
    }
    return dimacs ;
    
}

nfa * buildNFA(descriptionNode * d){
    nfa * NFA = malloc(sizeof(nfa)) ;

    NFA->states = stateCount(d) ;

    int * selfLoops = malloc(NFA->states * sizeof(int)) ;
    int * incomingOnes = malloc(NFA->states * sizeof(int)) ;
    int * incomingZeros = malloc(NFA->states * sizeof(int)) ;

    for (int i = 0 ; i < NFA->states ; i++){ // Initialize to Zero
        selfLoops[i] = 0 ;
        incomingOnes[i] = 0 ;
        incomingZeros[i] = 0 ;
    }

    // Assign to the NFA
    NFA->selfZeros = selfLoops ;
    NFA->inOnes = incomingOnes ;
    NFA->inZeros = incomingZeros ;

    NFA->selfZeros[0] = 1 ; // State zero always has a self-loop (set 1 for j = 0)
    for (int k = 0 ; k < d->val ; k++){ // set 2 for j = 0
        NFA->inOnes[k+1] = 1 ;
    }
    NFA->selfZeros[NFA->states - 1] = 1 ; // set 3 for j=t

    int l_LEQ_j = d->val ;
    descriptionNode * temp = d->next ;

    for (int j = 1 ; j < d->length ; j++){
        NFA->selfZeros[l_LEQ_j + j] = 1 ; // set 1

        for (int k = 0 ; k < temp->val ; k++){ // set 2
            NFA->inOnes[l_LEQ_j + j + k + 1] = 1 ;
        }

        NFA->inZeros[l_LEQ_j + j] = 1 ; // set 3

        l_LEQ_j += temp->val ;
        temp = temp->next ;
    }

    return NFA ;
}

int stateCount(descriptionNode * d){
    int count = 0 ;
    descriptionNode * temp = d ;
    while (temp != NULL){
        count += temp->val ;
        count += 1 ;
        temp = temp->next ;
    }
    return count ;
}

void freeDescription(descriptionNode * d){
    descriptionNode * temp = d ;
    descriptionNode * tempNext = NULL ;
    while (temp != NULL){
        tempNext = temp->next ;
        free(temp) ;
        temp = tempNext ;
    }
    return ;
}

void printNFA(nfa * n ){
    printf("States: %d\n",n->states) ;
    
    if (n->selfZeros[0] == 1){
        printf("(0,0,0)\n") ;
    } else {
        printf("SOMETHING IS WRONG!\n") ;
        return ;
    }

    for (int i = 1 ; i <= n->states ; i++){
        if (n->selfZeros[i] == 1){
            printf("(%d,0,%d)\n",i,i) ;
        }
        if (n->inOnes[i] == 1){
            printf("(%d,1,%d)\n",i-1,i) ;
        }
        if (n->inZeros[i] == 1){
            printf("(%d,0,%d)\n",i-1,i) ;
        }
    }
    return ;
}
