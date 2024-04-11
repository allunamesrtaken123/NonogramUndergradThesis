#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "mtwister.h"
#include "buf.h"

#define N 40 // The size of the board

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

//descriptionNode * generateDescription(double p,MTRand seed) ;

/*
stateCount: descriptionNode * -> int
stateCount(d) = s, the number of states in the automata representing d (as defined in section 2.3)
*/
int stateCount(descriptionNode * d) ;

//int * transitions(descriptionNode * d) ;

/*
buildNFA: descriptionNode * -> nfa *
buildNFA(d) = n, the automata representing d (as defined in section 2.3). Strings will be accepted by
n if they fit the description d (for any size line).
*/
nfa * buildNFA(descriptionNode * d) ;
void printNFA(nfa * n) ;

/*
buildConstraint: nfa * x int x int x descriptionNode * -> Buf
buildConstraint(n,stringVariables,variableIndex,d) = Ψ, where Ψ is satisfiable when there is a string input
to n that can be accepted. 

The parameter stringVariables is an array of the variables that correspond to the cells in the board that d 
is constraining (row 0 in an NxN board would be [x_0,...,x_{N-1}]). The parameter variableIndex is the minimum
 index of a fresh variable in Ψ.
*/
Buf buildConstraint(nfa * n, int * stringVars, int * varIndex, descriptionNode * d) ;

/*
clauseCount: descriptionNode * -> int
clauseCount(d) = c, the number of clauses in the CNF formula encoding the description d (as defined in section 2.3)
*/
int clauseCount(descriptionNode * d) ;

/*
formulaVarCount: descriptionNode * -> int
formulaVarCount(d) = v, the total number of variables in the CNF formula encoding the 
description d (as defined in section 2.3)
*/
int formulaVarCount(descriptionNode * d) ;

/*
uniqueVarCount: descriptionNode * -> int
uniqueVarCount(d) = v, the number of distinct variables in the CNF formula encoding the 
description d (as defined in section 2.3)
*/
int uniqueVarCount(descriptionNode * d) ;

/*
digits: int -> int
digits(i) = d, the number of digits in the base-10 representation of i
*/
int digits(int number) ;

/*
randomFilled: float x MTRand -> int *
randomFilled(p,seed) = B, an N*N-element binary vector representing a Nonogram board
*/
int * randomFilled(float p, MTRand r) ;

/*
transpose: int * -> int *
transpose(xs) = ys, where entry xs[i,j] == ys[j,i]
*/
int * transpose(int * matrixList) ;

/*
descriptionsFromBoard: int * -> descriptionNode **
descriptionsFromBoard(B) = Ds, an array of Nonogram line descriptions,
represented as descriptionNode linked lists
*/
descriptionNode ** descriptionsFromBoard(int * board) ;
void freeDescription(descriptionNode * d) ;
Buf emptyLine(int * stringVars) ;


int main(void){
    MTRand seed = seedRand(32) ;
    int densityCount = 1 ;
    for (float d = 0.03 ; d < 1.0; d = d + 0.03){ // Specify the start, stop, and step for board densities
        printf("%.2f\n",d) ;
        for (int b = 0 ; b < 500 ; b++){ // b is the number of boards
            // Fill the board
            int board[N*N] ;
            for (int i = 0 ; i < N*N ; i++){
                if (genRand(&seed) < d){
                    board[i] = 1 ;
                } else {
                    board[i] = 0 ;
                }
            } 
            
            // Calculate the number of variables and clauses that will be in the resulting formula
            int rowVars = 0 ; 
            int rowClauses = 0 ;
                // Generate Row Descriptions
            descriptionNode ** rowDescriptions = descriptionsFromBoard(board) ;
            for (int i = 0 ; i < N ; i++){
                if (rowDescriptions[i]->length != 0){ // If you don't have an empty line
                    //printDescription(rowDescriptions[i]) ;
                    int rowVars_i = uniqueVarCount(rowDescriptions[i]) ;
                    int rowClauses_i = clauseCount(rowDescriptions[i]) ;
                    //printf("Unique Variables: %d\t Clauses: %d\n",rowVars_i,rowClauses_i) ;
                    rowVars += rowVars_i ;
                    rowClauses += rowClauses_i ;
                } else { // Otherwise you just have a singleton clause for each variable in the line
                    rowClauses += N ;
                    //printf("<>\n") ;
                }  
            }
            int columnVars = 0 ; 
            int columnClauses = 0 ;
                // Generate Column Descriptions
            descriptionNode ** columnDescriptions = descriptionsFromBoard(transpose(board)) ;
            for (int i = 0 ; i < N ; i++){
                if (columnDescriptions[i]->length != 0){// If you don't have an empty line
                    //printDescription(columnDescriptions[i]) ;
                    int columnVars_i = uniqueVarCount(columnDescriptions[i]) ;
                    int columnClauses_i = clauseCount(columnDescriptions[i]) ;
                    //printf("Unique Variables: %d\t Clauses: %d\n",columnVars_i,columnClauses_i) ;
                    columnVars += columnVars_i ;
                    columnClauses += columnClauses_i ;
                } else { // Otherwise you just have a singleton clause for each variable in the line
                    columnClauses += N ;
                    //printf("<>\n") ;
                }
            }
            //printf("Total Clauses: %d\tTotal Variables: %d\n",rowClauses + columnClauses,N*N + rowVars + columnVars) ;
            //printf("\n") ;
            // file path to which the formula of the current iteration will be saved
            FILE * fp ; 
            char index[100] ;
            sprintf(index,"../Senior-Spring/Clause-Size-Check/%d %d.cnf",densityCount,b) ; 
            fp = fopen(index,"w") ;
            // Header for DIMACS format (https://jix.github.io/varisat/manual/0.2.0/formats/dimacs.html)
            fprintf(fp,"p cnf %d %d\n",N*N + rowVars + columnVars,rowClauses + columnClauses) ;


                // Let's actually write to file now for each description!
            int * varIndex = malloc(sizeof(int)) ;
            *varIndex = N*N+1 ;
                // Do the Rows First
            for (int i = 0 ; i < N ; i++){
                int stringVars[N] ;
                for (int j = 0 ; j < N ; j++){ // Get the string variables for the row being encoded
                    stringVars[j] = i*N + j + 1 ; 
                }
                if (rowDescriptions[i]->length != 0){ // If you don't have an empty row
                    // Construct the NFA
                    nfa * n = buildNFA(rowDescriptions[i]) ; 
                    // Construct the CNF formula for the NFA, storing it in a buffer
                    Buf constraint = buildConstraint(n,stringVars,varIndex,rowDescriptions[i]) ;
                    // Dump the buffer into the file
                    fprintf(fp,"%s",buf_data(constraint)) ;

                    // clean up after yourself...
                    free(n->inOnes) ;
                    free(n->inZeros) ;
                    free(n->selfZeros) ;
                    free(n) ;
                    free(constraint) ;
                } else { // If you do have an empty row
                    Buf constraint = emptyLine(stringVars) ;
                    fprintf(fp,"%s",buf_data(constraint)) ;
                    free(constraint) ;
                }  
            }
                // Then the Columns
            for (int i = 0 ; i < N ; i++){
                int stringVars[N] ;
                for (int j = 0 ; j < N ; j++){ // Get the string variables for the column being encoded
                    stringVars[j] = j*N + i + 1 ;
                }
                if (columnDescriptions[i]->length != 0){ // If you don't have an empty column
                    // Construct the NFA
                    nfa * n = buildNFA(columnDescriptions[i]) ;
                    // Construct the CNF formula for the NFA, storing it in a buffer
                    Buf constraint = buildConstraint(n,stringVars,varIndex,columnDescriptions[i]) ;
                    // Dump the buffer into the file
                    fprintf(fp,"%s",buf_data(constraint)) ;

                    // clean up after yourself...
                    free(n->inOnes) ;
                    free(n->inZeros) ;
                    free(n->selfZeros) ;
                    free(n) ;
                    free(constraint) ;
                } else { // If you do have an empty row
                    Buf constraint = emptyLine(stringVars) ;
                    fprintf(fp,"%s",buf_data(constraint)) ;
                    free(constraint) ;
                }  
            }
            // Clean Up Time!
            for (int i = 0 ; i < N ; i++){
                freeDescription(rowDescriptions[i]) ;
                freeDescription(columnDescriptions[i]) ;
            }
            free(rowDescriptions) ;
            free(columnDescriptions) ;
            fclose(fp) ;
        }
        densityCount += 1 ;
    }
    printf("\n") ;

    return 0 ;
    
}

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

/* NOT USED ---- (you need to generate an entire board, can't do it one line at a time)
descriptionNode * generateDescription(double p,MTRand seed){
    descriptionNode * head = malloc(sizeof(descriptionNode)) ;
    head->length = 0 ;
    int currentRun = 0 ;
    bool currentlyRunning = false ;

    for (int i = 0 ; i < N ; i++){
        if (genRand(&seed) < p){
            currentRun += 1 ;
            currentlyRunning = true ;
        } else {
            if (currentlyRunning){
                appendDescription(head,currentRun) ;
                currentlyRunning = false ;
                currentRun = 0 ;
            }
        }
    }

    if (currentlyRunning){
        appendDescription(head,currentRun) ;
    }
    if (head->tail == NULL){
        free(head) ;
        return NULL ;
    } else {
        return head ;
    }
} */

int stateCount(descriptionNode * d){
    /*
    Traverses the linked list, summing the number of links along with the val field of the links
    */
    int count = 0 ;
    descriptionNode * temp = d ;
    while (temp != NULL){
        count += temp->val ;
        count += 1 ;
        temp = temp->next ;
    }
    return count ;
}

/* NOT USED --- (incorporated into buildNFA)
int * transitions(descriptionNode * d){
    int states = stateCount(d) ; // Number of states in the NFA
    int * mat = malloc(states * states * sizeof(int)) ; // Malloc the transition set
    for (int i = 0 ; i < states * states ; i++){ // Initialize values to -1 to prevent errors when filling in with real values
        mat[i] = -1 ;
    }

    int l_LEQ_j = 0 ;

    mat[0] = 0 ; // Self-loop on zero from the first state back to the first state

    for (int k = 0 ; k < d->val ; k++){ // Transitions for the first run (j=0)
        mat[k*states + k + 1] = 1;
    }

    l_LEQ_j += d->val ;
    descriptionNode * temp = d->next ;
    for (int j = 1 ; j < d->length ; j++){
        int index1 = l_LEQ_j + j + (l_LEQ_j + j) * states ; // Set 1 for the transition set
        if (mat[index1] == -1){
            mat[index1] = 0 ;
        } else {
            mat[index1] = 2 ;
        }

        for (int k = 0 ; k < temp->val ; k++){ // Set 2 for the transition set
            int index2 = l_LEQ_j + j + (l_LEQ_j + j)*states + k*states + k + 1 ;
            if (mat[index2] == -1){
                mat[index2] = 1 ;
            } else {
                mat[index2] = 2 ;
            }
        }
        
        int index3 = l_LEQ_j + j + (l_LEQ_j + j-1)*(states) ; // Set 3 for the transition set
        if (mat[index3] == -1){
            mat[index3] = 0 ;
        }  else {
            mat[index3] = 2 ;
        }
        l_LEQ_j += temp->val ;
        temp = temp->next ;
    }
    mat[states*states - 1] = 0 ; // Set 3 for j=t

    return mat ;
}
*/
nfa * buildNFA(descriptionNode * d){
    nfa * NFA = malloc(sizeof(nfa)) ;

    NFA->states = stateCount(d) ; // Get the number of states

    // Get ready to build the automata
    int * selfLoops = malloc(NFA->states * sizeof(int)) ;
    int * incomingOnes = malloc(NFA->states * sizeof(int)) ;
    int * incomingZeros = malloc(NFA->states * sizeof(int)) ;

    for (int i = 0 ; i < NFA->states ; i++){ // Initialize to Zero (no transition)
        selfLoops[i] = 0 ;
        incomingOnes[i] = 0 ;
        incomingZeros[i] = 0 ;
    }

    // Assign to the NFA
    NFA->selfZeros = selfLoops ;
    NFA->inOnes = incomingOnes ;
    NFA->inZeros = incomingZeros ;

    // sets 1,2,3 are labelled in section 2.3
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

Buf buildConstraint(nfa * n, int * stringVars, int * varIndex, descriptionNode * d){
    // First we build up the variables that will be sampled from when building the formula

    int stateVars[(N+1) * n->states] ;
    for (int i = 0 ; i < (N+1)*n->states ; i++){
        stateVars[i] = *varIndex ;
        *varIndex = *varIndex + 1 ;
    }

    int transitionVars[2 * N * n->states] ;

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

    for (int k = 1 ; k < N ; k++){ // You can then copy over from k=1, incrementing the variable counter each time
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
    int bufferSize = 4*clauseCount(d) + (digits(*varIndex) + 2)*formulaVarCount(d) ;
    Buf dimacs = buf_new(bufferSize) ;

    // Build Up the Buffer!
    for (int k = 0 ; k < N ; k++){
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
    // Sixth Constraint
    for (int i = 1 ; i < n->states ; i++){
        //printf("(-%d) ",stateVars[i]) ;
        buf_append(dimacs, "-%d 0\n",stateVars[i]) ;
    }
    //printf("\n") ;
    for (int i = n->states * N ; i < n->states * (N+1) - 1 ; i++){
        //printf("(-%d) ",stateVars[i]) ;
        buf_append(dimacs, "-%d 0\n",stateVars[i]) ;
    }
    //printf("\n\n") ;
    //printf("BUFFER:\n%s",buf_data(dimacs)) ;
    return dimacs;
    
}

int clauseCount(descriptionNode * d){
    int t = 0 ; 
    int s = 0 ;
    descriptionNode * temp = d ;
    // Traverse the description linked list, counting the number of runs and the total length of the runs
    while (temp != NULL){
        s += temp->val ;
        t += 1 ;
        temp = temp->next ;
    }
    return (5*N+2)*(t+1+s) - 4 ; // return the recurrence (section 2.4)
}
int formulaVarCount(descriptionNode * d){
    int t = 0 ; 
    int s = 0 ;
    descriptionNode * temp = d ;
    // Traverse the description linked list, counting the number of runs and the total length of the runs
    while (temp != NULL){
        s += temp->val ;
        t += 1 ;
        temp = temp->next ;
    }
    return (14*N+2)*t+8*N-2+(11*N+2)*s ; // return the recurrence (section 2.4)
}
int uniqueVarCount(descriptionNode * d){
    int t = 0 ; 
    int s = 0 ;
    descriptionNode * temp = d ;
    // Traverse the description linked list, counting the number of runs and the total length of the runs
    while (temp != NULL){
        s += temp->val ;
        t += 1 ;
        temp = temp->next ;
    }
    return (2*N+1)*(t+s) + N ; // return the recurrence (section 2.4)
}
int digits(int number){
    int copy = number ;
    return (int) log10(copy) + 1 ;
}

int * randomFilled(float p, MTRand seed){
    int * tiles = malloc(sizeof(int)*N*N) ;
    for (int i = 0 ; i < N*N ; i++){
        if (genRand(&seed) < p){
            tiles[i] = 1 ;
        } else {
            tiles[i] = 0 ;
        }
    }
    return tiles ;
}

int * transpose(int * matrixList){
    for (int i = 0 ; i < N ; i++){
        for (int j = (N+1)*i + 1 ; j < (i+1)*N ; j++){
            int t = matrixList[j] ;
            matrixList[j] = matrixList[(j%N)*N + i] ;
            matrixList[(j%N)*N + i] = t ;
        }
    }
    return matrixList ;
}

descriptionNode ** descriptionsFromBoard(int * board){
    descriptionNode ** descriptions = malloc(N*sizeof(descriptionNode)) ;
    // There are N rows or N columns to get descriptions for
    for (int i = 0 ; i < N ; i++){
        descriptionNode * head = malloc(sizeof(descriptionNode)) ;
        head->length = 0 ; // start off with no description elements
        descriptions[i] = head ;
        int currentRun = 0 ; // zero-length run to start
        bool currentlyRunning = false ; // not in a run to start

        for (int j = i*N ; j < (i+1)*N ; j++){
            if (board[j] == 1){ // If you see a filled cell
                currentRun += 1 ;
                currentlyRunning = true ;
            } else {
                // If you had been seeing filled cells, you've reached the end of a run and can add it to the description
                if (currentlyRunning){ 
                    appendDescription(descriptions[i],currentRun) ;
                    currentlyRunning = false ;
                    currentRun = 0 ;
                }
            }
        }
        // If you end and are running, add that to the description as well
        if (currentlyRunning){
            appendDescription(descriptions[i],currentRun) ;
        }
    }
    return descriptions ;
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

Buf emptyLine(int * stringVars){
    Buf dimacs = buf_new(N*(digits(stringVars[N-1]) + 5)) ;
    for (int i = 0 ; i < N ; i++){
        buf_append(dimacs,"-%d 0\n",stringVars[i]) ;
    }
    return dimacs ;
    
}