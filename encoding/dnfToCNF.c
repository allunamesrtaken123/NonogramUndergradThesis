#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "mtwister.h"

#define N 8

// Struct Declarations
typedef struct node node ;
typedef struct DNFnode DNFnode ;
typedef struct CNFnode CNFnode ;
typedef struct DNFtreeNode DNFtreeNode ;
typedef struct CNFtreeNode CNFtreeNode ;
typedef struct literalNode literalNode ;
typedef struct literalIndexNode literalIndexNode ;

/*
Descriptions are implemented as a linked list of node structs. To allow for constant time appending,
each node tracks the tail, as well as the length. Each field:
    
    val --> the integer value of the description
    next --> the next description element
    tail --> the final description element (NULL if not set)
    length --> the length of the description

*/
struct node {
    int val ;
    int len ;
    node * next ;
    node * tail ;
} ;

/*
Terms in a DNF formula are implemented using this DNFnode struct. A linked list of these structs
implements a DNF formula. Each field:

    next --> the next term
    tail --> the final term (for constant time appending)
    scaling --> the row/line index that the DNF formula constrains
    term --> an array with values of -1,0, and 1, where non-zero values index the variables that occur in the formula
*/
struct DNFnode {
    DNFnode * next ;
    DNFnode * tail ;
    int scaling ;
    int * term ;
} ;

/*
Clauses in a CNF formula are implemented using this CNFnode struct. A linked list of these structs
implements a CNF formula. For sparse formulas, testing for subsumption is much more efficient by
iterating over the indices linked list struct rather than going through the entire clause array
to see which indices are non-zero.

Each field:

    next --> the next clause
    tail --> the final clause.
    clause --> an array with values of -1,0, and 1, where non-zero values index the variables that occur in the formula
    len --> the number of literals in the clause
    indices --> a linked list, where the value of each node is an index in clause that is non-zero.

*/
struct CNFnode {
    CNFnode * next ;
    CNFnode * tail ;
    int * clause ;
    int len ;
    literalIndexNode * indices ;
} ;

/*
To store the computed DNF formulas for line length-description combinations, we use a tree. To identify the formula for
a description [d_0,...,d_k] in line size l, traverse the tree by description element, and once the description has been
entirely processed, retrieve the l-1 element of DNFtreeNode.constraints. Each field:

    constraints --> an array of DNF formulae, where the index-i element is the DNF formula for the fillings of the
                    description in a line of length i+1 reached by the traversal of the tree ending at the struct
    children --> the children nodes
    scaled --> true if the N-1th element of constraints has been scaled from a -1/0/1 array to be integers indicating variables
*/
struct DNFtreeNode {
    DNFnode ** constraints ;
    DNFtreeNode ** children ;
    bool scaled ;
} ;

/*
Stores the previously-computed CNF formulae. Since you only ever need CNF formulae for full-length lines, the constraints
are now just a single CNF formula that is always already scaled, rather than an array of subproblems. Each field:

    children --> childred nodes
    cnf --> the CNF formula encoding the description accumulated by traversing the tree to the node
*/
struct CNFtreeNode {
    CNFtreeNode ** children ;
    CNFnode * cnf ;
} ;

/*
Used in the memory-efficient encoding of DNF into CNF, where the count of each literal is useful. Each field:

    literal --> the literal the node is tracking
    frequency --> the number of times literal occurs in the formula
*/
struct literalNode {
    int literal ;
    int frequency ;
} ;

/*
Allows for more efficient subsumption testing. Used as a linked structure, where the chain collectively tracks the
indices of the literals in the CNF formula with which it is associated. Each field:

    index --> the index of the literal with which this node is associated
    next --> the next literal index
    tail --> the final literal index
*/
struct literalIndexNode {
    int index ;
    literalIndexNode * next ;
    literalIndexNode * tail ;
} ;

/* 
 ****************************************************************
 *                                                              *
 *                      Function Declarations                   *
 *                                                              *
 ****************************************************************
*/

// Getting the Indices of the filled tiles
/*
randomFilled: int x MTRand -> int *
randomFilled(t,seed) = B, an N*N-element binary vector representing a Nonogram board
*/
int * randomFilled(int t, MTRand r) ;
void printFilled(int n, int * filled) ;

//--------Generate Row and Column Descriptions--------
/*
append: node ** x int -> void
appendDescription(
    [[d_00,d_01,...,d_0a],[d_10,d_12,...,d_1b],...,[d_i0,d_i2,...,d_ik],...,[d_i0,d_i2,...,d_iN]],i,d) = 
    [[d_00,d_01,...,d_0a],[d_10,d_12,...,d_1b],...,[d_i0,d_i2,...,d_ik,d],...,[d_i0,d_i2,...,d_iN]]
*/
void append(node ** desc,int index, node * p) ;

// Generates row descriptions from an input board
void genRowDescriptions(int n, int * filled, node *[N]) ;

// Generates row descriptions from an input board
void genColumnDescriptions(int n, int * filled, node *[N]) ;

// True if the row is empty (no filled cells), false otherwise
bool isEmpty(node * row) ;
void printDescription(node *[N]) ;


//--------DNF Dynamic Programming--------
/* True if there is a formula for the description description in a length of tiles in 
the tree with root root, false otherwise */
bool inDNFtree(node * description, int tiles, DNFtreeNode * root) ;

/*
Preconditions:
    1) description is not the description for the empty line
    2) member(description,tiles,root) = true
*/
DNFnode * retrieve(node * description, int tiles, DNFtreeNode * root) ;
void printDNF(struct DNFnode * dnf) ;
/*
Preconditions:
    1) description is not the description for the empty line
    2) member(description,tiles,root) = false

Returns the DNF formula for the fillings of the description description in a
line of length tiles.
*/
DNFnode * build(node * description, int tiles, DNFtreeNode * root) ;

/*
addFirst([a_0,...,a_i],[[d_00,d_01,...d_0s,...,d_0e,...,d_0j],[d_10,d_11,...d_1s,...,d_1e,...,d_1j],...,[d_k0,d_k1,...d_ks,...,d_ke,...,d_kj]],s,e) = 
    [[d_00,d_01,...d_0{s-1},a_0,...,a_i,d_0{e+1},...,d_0j],[d_10,d_11,...d_1{s-1},a_0,...,a_i,d_1{e+1},...,d_1j],...,[d_k0,d_k1,...d_k{s-1},a_0,...,a_i,d_k{e+1},...,d_kj]]

Copies a_0,...,a_i into the values between indices s and e of each of the arrays in addTo.
*/
DNFnode * addFirst(int * toAdd, DNFnode * addTo, int startI, int endI) ;

// disjuncts the two formulas m1 and m2
struct DNFnode * merge(DNFnode * m1, DNFnode * m2) ;
void printSingleDescription(node * description) ;

// sum(description) + length(description) - 1 <= cells
bool notValidDescription(node * description, int cells) ;

// Inserts constraint into the the tree rooted at root
void insert(node * description, int tiles, DNFnode * constraint, DNFtreeNode * root) ;

// Scales the DNF constraints of full length formulae in the tree to be variable indices rather than -1/0/1 arrays
void scaleFullLength(DNFtreeNode * treeNode) ;

// Prints tree for debugging
void printNodeRec(node * description, DNFtreeNode * treeNode) ;

//--------CNF Dynamic Programming--------
// True if there is a CNF formula for description in the tree rooted at root
bool inCNFtree(node * description, CNFtreeNode * root) ;
// Inserts cnf into the tree rooted at root as the formula for the description
void insertCNF(node * description, CNFnode * cnf, CNFtreeNode * root) ;
//explode([x_0,...,x_n]) = [[x_0],[x_1],...,[x_n]]
CNFnode * explode(DNFnode * dnf, int * accumulator) ;
/* converts DNF to CNF according to the rules:
    (F1 ^ F2) v F3 <=> (F1 v F3) ^ (F2 v F3)
    F1 v (F2 ^ F3) <=> (F1 v F2) ^ (F1 v F3)
*/
CNFnode * f(DNFnode * dnf, int * accumulator) ;
/*
incorporate(v,i,[[x_00,...,x_0i,...,x_0N],[x_10,...,x_1i,...,x_1N],...,[x_m0,...,x_mi,...,x_mN]]) = 
    [[x_00,...,x_0{i-1},v,x_0{i+1},...,x_0N],[x_10,...,x_1{i-1},v,x_1{i+1},...,x_1N],...,[x_m0,...,x_m{i-1},v,x_m{i+1},...,x_mN]]
*/
CNFnode * incorporate(int literal, int index, CNFnode * cnf) ;
// mergeCNF(first,second) = first ^ second
CNFnode * mergeCNF(CNFnode * first, CNFnode * second) ;
// True if literalSet(sub) ⊆ literalSet(sub)
bool isSubConstraint(struct CNFnode * sub, struct CNFnode * super) ;
// Removes subsumed clauses from cnf
struct CNFnode * removeRedundant(struct CNFnode * cnf) ;
void printCNF(node * description, CNFtreeNode * treeNode) ;
// Removes empty clauses (no literals)
CNFnode * removeEmpty(CNFnode * cnf) ;
// True if cnf->clause is all zeros
bool emptyClause(CNFnode * cnf) ;
// Removes subsumed clauses
CNFnode * subsumption(CNFnode * cnf, int clauseLength) ;
// tests for subsumption (literalSet(subsumer) ⊆ literalSet(subsumed)) but allows for flexible clause length
bool canSubsume(CNFnode * subsumer, CNFnode * subsumed, int clauseLength) ;
// Properly sets the length field of the CNFnode structs
CNFnode * setLength(CNFnode * formula) ;
// Scales the variables in scaleFrom to be propely indexed for the variables in row index
int * scaleRow(int * scaleFrom, int * fill, int index) ;
// Scales the variables in scaleFrom to be propely indexed for the variables in column index
int * scaleColumn(int * scaleFrom, int * fill, int index) ;
CNFnode * copyCNFscaled(node * description, CNFtreeNode * root, int index, char line) ;
CNFnode * emptyLineCNF() ;
void copySmallToBig(int * small, int * big, int index, char line) ;

//--------Memory Efficient Conversion from DNF to CNF--------
// Converts a DNF formula into a logically equivalent CNF
CNFnode * DNFtoCNF(DNFnode * dnf) ;
// Calculates the frequency of each literal dnf
literalNode * getFrequencies(DNFnode * dnf, int * termCount) ;
// addToLedger(clause,ledger) = clause ^ ledger
void addToLedger(int * clause, CNFnode * ledger) ;
//ledgerSubsumes(c,[c_0,c_1,...,c_l]) = isSubsumed(c_0,c) v isSubsumed(c_1,c) v ... v isSubsumed(c_l,c)
bool ledgerSubsumes(int * potentialClause, CNFnode * ledger) ;
bool isSubsumed(int * subsumed, CNFnode * subsumer) ;


int main(void){
    

    // INITIALIZE DNF TREE
    DNFtreeNode * DNFDP = malloc(sizeof(DNFtreeNode)) ;
    DNFnode ** constraints = malloc(N*sizeof(DNFnode *));
    DNFDP->constraints = constraints ;
    DNFtreeNode ** childArray = malloc(N*sizeof(DNFtreeNode *));
    DNFDP->children = childArray ;
    DNFDP->scaled = false ;

    /*
    BASE CASES
    
    The base cases are all of the singleton descriptions (1,...,N) and the DNF constraints
    for all line sizes (1,...,N). For a description r in a line of c cells:
        if r > c -->    no fillings (NULL)
        if r = c -->    the final r cells are positive, first N-r negative
        if r > c -->    first N-c negative, then r positive, then remaining c-r negative concatenated
                        with the fillings for r in c-1
    */
    for (int r = 0 ; r < N ; r++){
        DNFtreeNode * child = malloc(sizeof(DNFtreeNode)) ;
        child->scaled = false ;
        DNFnode ** childConstraints = malloc(N*sizeof(DNFnode *)) ;
        child->constraints = childConstraints ;
        DNFtreeNode ** grandchildren = malloc(N*sizeof(DNFtreeNode *)) ;
        for (int i = 0 ; i < N ; i++){grandchildren[i] = NULL ;}
        child->children = grandchildren ;
        /* This child will be the fillings for a run of size r across 
        all possible line sizes (c = 1,2,...,N)
        
        Since it's zero-indexed the first element in the array is index zero
        but corresponds to a line of size one. Same for r.
        */
        for (int c = 0 ; c < N ; c++){
            if (r > c){ // Can't fit a run of r+1 in c+1 cells
                child->constraints[c] = NULL ;
            } else if (r == c){ // final r+1 cells positive, first N-r-1 negative
                DNFnode * f = malloc(sizeof(DNFnode)) ;
                f->tail = f ;
                f->next = NULL ;
                int * indicator = malloc(N*sizeof(int)) ;
                f->term = indicator ;
                for (int j = 0 ; j < N ; j++){
                    if (j < N-r-1){
                        f->term[j] = -1 ;
                    } else{
                        f->term[j] = 1 ;
                    }
                }
                child->constraints[c] = f ;
            } else {
                // Construct the one new filling
                DNFnode * fNew = malloc(sizeof(DNFnode)) ;
                fNew->tail = fNew ;
                fNew->next = NULL ;
                int * indicator = malloc(N*sizeof(int)) ;
                fNew->term = indicator ;
                for (int j = 0 ; j < N ; j++){
                    if (j < N-c-1){
                        fNew->term[j] = -1 ;
                    } else if (j < N-c+r){
                        fNew->term[j] = 1 ;
                    } else {
                        fNew->term[j] = -1 ;
                    }
                }
                // Combine with fillings from prior sizes
                fNew->next = child->constraints[c-1] ;
                fNew->tail = child->constraints[c-1]->tail ;
                child->constraints[c] = fNew ;
            }
        }
        DNFDP->children[r] = child ;
    }

    // INITIALIZE CNF TREE
    CNFtreeNode * CNFDP = malloc(sizeof(CNFtreeNode)) ;
    CNFDP->cnf = emptyLineCNF() ;
    CNFtreeNode ** childCNF = malloc(N*sizeof(CNFtreeNode *));
    CNFDP->children = childCNF ;
    
    int boards = 0 ;
    for (int d = 4 ; d < N*N ; d+= 4){
        for (int b = 0 ; b < 2 ; b++){
            MTRand seed = seedRand(31 + boards) ;
            printf("\nDensity - %d \t Board - %d\n", d,b) ;
            // Fill the board randomly
            int * t ;
            t = randomFilled(d,seed) ;
            printFilled(N,t) ;
            // Build up the row descriptions
            node * rowDescriptions[N] ;
            for (int i = 0 ; i < N ; i++){
                node *row = malloc(sizeof(node));
                rowDescriptions[i] = row ;
            }
            genRowDescriptions(N,t, rowDescriptions) ;

            // Build up the column descriptions
            node * columnDescriptions[N] ;
            for (int i = 0 ; i < N ; i++){
                node *column = malloc(sizeof(node));
                columnDescriptions[i] = column ;
            }
            genColumnDescriptions(N,t, columnDescriptions) ;

            /*
            printf("Row\t Description\n") ;
            printDescription(rowDescriptions) ;
            printf("Column\t Description\n") ;
            printDescription(columnDescriptions) ;
            */
            // Building up the DNF Tree with the Row and Column Descriptions
            for (int index = 0 ; index < N ; index++){
                if (rowDescriptions[index]->val != 0){
                    if (!inDNFtree(rowDescriptions[index],N,DNFDP)){ // If not in the tree, need to add it
                        //printf("Starting building for row %d\t", index + 1) ;
                        node * tempRow = rowDescriptions[index] ;
                        /*printf("Description: < ") ;
                        while (tempRow != NULL){
                            printf("%d ",tempRow->val) ;
                            tempRow = tempRow->next ;
                        } */
                        //printf(">\n") ;
                        insert(rowDescriptions[index],N,build(rowDescriptions[index],N,DNFDP),DNFDP) ;
                    }
                }
                
                if (columnDescriptions[index]->val != 0){ // If not in the tree, need to add it
                    if (!inDNFtree(columnDescriptions[index],N,DNFDP)){
                        //printf("Starting building for column %d\n", index + 1) ;
                        insert(columnDescriptions[index],N,build(columnDescriptions[index],N,DNFDP),DNFDP) ;
                    }
                } 
            }

            scaleFullLength(DNFDP) ; // Change full length DNF from indicators (0-1) to boolean variables (positive integers)
            //printf("Scaled Up the DNF formulae\n") ;
            //printf("DNF Tree Grown\t") ;
            
            // Build up CNF tree
            for (int index = 0 ; index < N ; index++){
                if (rowDescriptions[index]->val != 0){ // If the description is not the empty description
                    if (!inCNFtree(rowDescriptions[index],CNFDP)){ // If not in the tree, need to add it
                        //printf("Starting building CNF for row %d\n", index + 1) ;
                        int accumulator[N] ;
                        for (int i = 0 ; i < N ; i++){accumulator[i] = 0 ;} // Initialize to Zero
                        
                        insertCNF(rowDescriptions[index],DNFtoCNF(retrieve(rowDescriptions[index],N,DNFDP)),CNFDP) ;
                        //printCNF(rowDescriptions[index],CNFDP) ;
                    }
                }
                
                if (columnDescriptions[index]->val != 0){ // If the description is not the empty description
                    if (!inCNFtree(columnDescriptions[index],CNFDP)){ // If not in the tree, need to add it
                        //printf("Starting building CNF for column %d\n", index + 1) ;
                        int accumulator[N] ;
                        for (int i = 0 ; i < N ; i++){accumulator[i] = 0 ;} // Initialize to Zero
                        
                        insertCNF(columnDescriptions[index],DNFtoCNF(retrieve(columnDescriptions[index],N,DNFDP)),CNFDP) ;
                        //printCNF(columnDescriptions[index],CNFDP) ;
                    }
                } 
            }
            //printf("CNF Tree Grown\n") ;
            CNFnode * longFormula = copyCNFscaled(rowDescriptions[0],CNFDP,0,'r') ;
            for (int i = 1 ; i < N ; i++){
                CNFnode * nextPortion = copyCNFscaled(rowDescriptions[i],CNFDP,i,'r') ;
                longFormula->tail->next = nextPortion ;
                longFormula->tail = nextPortion->tail ;
            }
            
            for (int i = 0 ; i < N ; i++){
                CNFnode * nextPortion = copyCNFscaled(columnDescriptions[i],CNFDP,i,'c') ;
                longFormula->tail->next = nextPortion ;
                longFormula->tail = nextPortion->tail ;
            }
            /*printf("Big Guy\n") ;
            CNFnode * printTemp = longFormula ;
            while (printTemp != NULL){
                printf("< ") ;
                for (int i = 0 ; i < N*N ; i++){
                    printf("%d ", printTemp->clause[i]) ;
                    
                }
                printf(">\n") ;
                printTemp = printTemp->next ;
            }*/
            //printf("Subsumption Time!\n") ; 
            longFormula = subsumption(longFormula,N*N) ;
            /*
            CNFnode * printTemp2 = longFormula ;
            while (printTemp2 != NULL){
                printf("< ") ;
                for (int i = 0 ; i < N*N ; i++){
                    printf("%d ", printTemp2->clause[i]) ;
                    
                }
                printf(">\n") ;
                printTemp2 = printTemp2->next ;
            } */
            
            int fixedLiterals[N*N] ;
            for (int i = 0 ; i < N*N ; i++){fixedLiterals[i] = 0 ;}
            CNFnode * cnf = longFormula ;
            while (cnf != NULL){
                int fixedLiteral = 0 ;
                for (int i = 0 ; i < N*N ; i++){
                    if (cnf->clause[i] != 0){
                        if (fixedLiteral == 0){
                            fixedLiteral = i+1 ;
                        } else {
                            fixedLiteral = -1 ;
                            break ;
                        }
                    }
                }
                if (fixedLiteral != -1){
                    fixedLiterals[fixedLiteral - 1] = cnf->clause[fixedLiteral - 1] ;
                }
                cnf = cnf->next ;
            }
            /*printf("Fixed Literal Indicator:\n[ ") ;
            for (int i = 0 ; i < N*N ; i++){printf("%d ",fixedLiterals[i]) ;}
            printf("]\n") ;*/
            
            CNFnode * tempFixedCleaner = longFormula ;
            while (tempFixedCleaner != NULL){
                for (int i = 0 ; i < N*N ; i++){
                    if (tempFixedCleaner->clause[i] != 0 && tempFixedCleaner->clause[i] == -1*fixedLiterals[i]){
                        tempFixedCleaner->clause[i] = 0 ;
                    }
                }
                tempFixedCleaner = tempFixedCleaner->next ;
            }
            /*printf("After Removing the Negation of Fixed Literals\n") ;
            CNFnode * printTemp3 = longFormula ;
            while (printTemp3 != NULL){
                printf("< ") ;
                for (int i = 0 ; i < N*N ; i++){
                    printf("%d ", printTemp3->clause[i]) ;
                    
                }
                printf(">\n") ;
                printTemp3 = printTemp3->next ;
            } */
            longFormula = subsumption(longFormula,N*N) ;
            /*printf("After Final Subsumption\n") ;
            CNFnode * finalPrint = longFormula ;
            while (finalPrint != NULL){
                printf("< ") ;
                for (int i = 0 ; i < N*N ; i++){
                    printf("%d ", finalPrint->clause[i]) ;
                    
                }
                printf(">\n") ;
                finalPrint = finalPrint->next ;
            } */
            int clauses = 0 ;
            CNFnode * clauseCounter = longFormula ;
            while (clauseCounter != NULL){
                clauses += 1 ;
                clauseCounter = clauseCounter->next ;
            }
            
            
            FILE * fp ;
            char index[50];
            sprintf(index,"8x8 V2 Testing/density-%d board-%d.cnf",d, b) ;
            
            fp = fopen(index,"w") ;
            fprintf(fp, "p cnf %d %d\n", N*N, clauses) ;

            CNFnode * writeTemp = longFormula ;
            while (writeTemp != NULL){
                for (int i = 0 ; i < N*N ; i++){
                    if (writeTemp->clause[i] != 0){
                        fprintf(fp,"%d ", writeTemp->clause[i]) ;
                    }
                }
                fprintf(fp,"0\n") ;
                writeTemp = writeTemp->next ;
            }
            fclose(fp) ; 
            CNFnode * freeTemp = longFormula ;
            CNFnode * nextFree = longFormula->next ;
            while (nextFree != NULL){
                free(freeTemp->clause) ;
                free(freeTemp) ;
                freeTemp = nextFree ;
                nextFree = nextFree->next ;
            }
            free(freeTemp->clause) ;
            free(freeTemp) ;
            boards += 1 ;
        }
    }
    
    
    return 0 ;
}



int * randomFilled(int t, MTRand r){
    // Initialize the board as empty (all cells zero)
    int * tiles = malloc(sizeof(int)*N*N) ;
    for (int i = 0 ; i < N*N ; i++){tiles[i] = 0 ;}
    int filled = 0 ;
    while (filled < t){
        int randIndex = genRand(&r)*N*N ;
        if (randIndex < N*N && tiles[randIndex] == 0){
            tiles[randIndex] = 1 ;
            filled += 1 ;
        } 
    }
    return tiles ;
}

void printFilled(int n, int * filled){
    for (int i = 0 ; i < n+2 ; i++){printf("-") ;}
    printf("\n") ;
    for (int i = 0 ; i < n*n ; i++){
        if (i % n == 0){printf("|") ;}
        if (filled[i] == 1){printf("*") ;}else{printf(" ") ;}
        if ((i+1) % n == 0){printf("|\n") ;}
    }
    for (int i = 0 ; i < n+2 ; i++){printf("-") ;}
    printf("\n");
    return ;
}

void genRowDescriptions(int n, int * filled, node *description[N]){
    int tileIndex = 0 ;

    while (tileIndex < n*n){
        if (filled[tileIndex] == 1){
            // tileIndex is at the start of a run, so get ready to increment
            int j = tileIndex ;
            int run = 0 ;
            node * p = malloc(sizeof(node)) ;

            // Find the length of the run
            while (filled[j] == 1){
                // Don't want to allow runs over multiple rows
                if (j/n != tileIndex/n){
                    break ;
                }
                j += 1 ;
                run += 1 ;
            }

            // Increment
            p->val = run ;
            append(description,tileIndex/n, p) ;
            tileIndex = j ;
        } else {
            tileIndex += 1 ;
        }
    }
    return ;
}

void genColumnDescriptions(int n, int * filled, node *descriptions[N]){
    int tileIndex = 0 ;

    for (int column = 0 ; column < n ; column++){
        int j = column ;
        while (j < n*n){
            if (filled[j] == 1){
                int k = j ;
                int run = 0 ;
                node * p = malloc(sizeof(node)) ;

                while (filled[k] && k < n*n){
                    k += n ;
                    run += 1 ;
                }
                j = k ;
                p->val = run ;
                append(descriptions, column, p) ;
            } else{
                j += n ;
            }
        }
    }
    return ;
}

void append(node * desc[N],int index, node * p){
    if (isEmpty(desc[index])){
        desc[index] = p ;
        p->next = NULL ;
        p->tail = p ;
    } else {
        if (desc[index]->next == NULL){
            desc[index]->next = p ;
            desc[index]->tail = p ;
        } else{
            p->next = desc[index]->tail->next ;
            desc[index]->tail->next = p ;
            desc[index]->tail = p ;
        } 
    }
    return ;
}

bool isEmpty(node * row){return (row->val == 0) ;}

void printDescription(node * p[N]){
    for (int i = 0 ; i < N ; i++){
        node * p2 = p[i] ;
        printf("%d\t", i+1) ;
        while (p2 != NULL){
            printf("%d",p2->val) ;
            p2 = p2->next ;
        }
        printf("\n") ;
    }
    return ;
}


/* 
 ****************************************************************
 *                                                              *
 *                      DNF Dynamic Programming                 *
 *                                                              *
 ****************************************************************
*/



bool inDNFtree(node * description, int tiles, DNFtreeNode * root){
    node * temp = description ;
    DNFtreeNode * head = root ;

    /* 
    Move through the tree to the node corresponding to the node
    that we care about. Check that the new head is not NULL each
    time to make sure we don't get a seg fault.
    */
    while (temp != NULL){
        if (head == NULL){
            return false ;
        }
        head = head->children[temp->val - 1] ;
        temp = temp->next ;
    }
    /* 
    After the loop we want to see if the constraint has
    been created for the line size and description combination.
    First we have to check that the node we iterated to is not
    NULL, as trying to access its constraints will result in a
    seg fault if so.
    */
    if (head == NULL){
        return false ;
    }
    // Now we can finally check if the constraint has been created yet
    return head->constraints[tiles - 1] != NULL ;
}

DNFnode * retrieve(node * description, int tiles, DNFtreeNode * root){
    // Walk through the tree
    struct node * temp = description ;
    struct DNFtreeNode * head = root ;
    while (temp != NULL){
        head = head->children[temp->val - 1] ;
        temp = temp->next ;
    }
    // Return the relevant constraint
    return head->constraints[tiles - 1] ;
}

void printDNF(DNFnode *dnf){
    int * hd = dnf->term ;
    printf("<") ;
    for (int i = 0 ; i < N ; i++){
        printf("%d ",hd[i]) ;
    }
    printf(">\n") ;
    return ; 
}
DNFnode * build(node * description, int tiles, DNFtreeNode * root){
    if (notValidDescription(description,tiles)){
        return NULL ; // There are no ways to fill an invalid description
    } 
    
    /* 
    Find all the ways of filling <tiles> tiles with the description <description> if
    you start the first element of the description at the first tile remaining
    */

    DNFnode * component1 ;

    if (!inDNFtree(description->next,tiles - description->val - 1,root)){
        // We're adding to the tree!
        insert(description->next,tiles - description->val - 1,build(description->next,tiles - description->val - 1,root),root) ;
    }

    component1 = retrieve(description->next,tiles - description->val - 1,root) ;

    int toAdd1[N] ;
    for (int i = N-tiles ; i < N-tiles + description->val ; i++){
        toAdd1[i] = 1 ;
    }
    toAdd1[N - tiles + description->val] = -1 ;

    /* 
    Find all the ways of filling <tiles> tiles with the description <description> if
    you don't start the first element of the description at the first tile remaining
    */

    DNFnode * component2 ;
    if (!inDNFtree(description,tiles - 1,root)){
        insert(description,tiles - 1,build(description,tiles - 1, root),root) ;
    }

    component2 = retrieve(description,tiles - 1,root) ;

    int toAdd2[N] ;
    toAdd2[N-tiles] = -1 ;

    DNFnode * constraint = malloc(sizeof(DNFnode)) ;
    constraint = merge(addFirst(toAdd1,component1,N - tiles,N - tiles + description->val + 1),addFirst(toAdd2,component2,N - tiles,N - tiles + 1)) ;
    return constraint ;
}

DNFnode * addFirst(int * toAdd, DNFnode * addTo, int startI, int endI){
    if (addTo == NULL){
        return NULL ;
    }
    // Copy over addTo so I can copy over the information in toAdd
    DNFnode * ret = malloc(sizeof(DNFnode)) ;
    ret->tail = ret ;
    int * indicator = malloc(N*sizeof(int)) ;
    ret->term = indicator ;
    
    for (int j = 0 ; j < N ; j++){
        ret->term[j] = addTo->term[j] ;
    }
    DNFnode * prev = ret ; 
    DNFnode * curr = addTo->next ;
    while (curr != NULL){
        DNFnode * new = malloc(sizeof(DNFnode)) ;
        new->tail = new ;
        prev->next = new ;
        ret->tail = new ;

        int * indicator = malloc(N*sizeof(int)) ;
        new->term = indicator ;
        
        for (int i = 0 ; i < N ; i++){
            new->term[i] = curr->term[i] ;
        }

        prev = prev->next ;
        curr = curr->next ;
    }

    DNFnode * tempDNF = ret ;
    // Add toAdd into all of the constraints
    while (tempDNF != NULL){
        for (int i = startI ; i < endI ; i++){
            tempDNF->term[i] = toAdd[i] ;
        }
        tempDNF = tempDNF->next ;
    }
    return ret ;
}

DNFnode * merge(DNFnode * first, DNFnode * second){
    if (first == NULL){
        return second ;
    } else if (second == NULL){
        return first ;
    } else{
        first->tail->next = second ;
        first->tail = second->tail ;
        return first ;
    } 
}

void printSingleDescription(node * description){
    node * temp = description ;
    printf("<") ;
    while (temp != NULL){
        printf("%d ", temp->val) ;
        temp = temp->next ;
    }
    printf(">\n") ;
    return ;
}

bool notValidDescription(struct node * description, int cells){
    int s = 0 ;
    int l = 0 ;
    struct node * p = description ;
    while (p != NULL){
        s += p->val ;
        l += 1 ;
        p = p->next ;
    }
    return s + l - 1 > cells ;
}

void insert(node * description, int tiles, DNFnode * constraint, DNFtreeNode * root){
    // Traverse the tree
    node * temp = description ;
    DNFtreeNode * head = root ;
    DNFtreeNode * chaser = NULL ;
    node * tempPrev = NULL ;

    while (temp != NULL){
        if (head == NULL){
            DNFtreeNode * newTreeNode = malloc(sizeof(DNFtreeNode)) ;
            newTreeNode->scaled = false ;
            DNFtreeNode ** children = malloc(N*sizeof(DNFtreeNode *)) ;
            for (int i = 0 ; i < N ; i++){children[i] = NULL ;}
            newTreeNode->children = children ;

            DNFnode ** constraints = malloc(sizeof(DNFnode *) * N) ;
            for (int i = 0 ; i < N ; i++){constraints[i] = NULL ;}
            newTreeNode->constraints = constraints ;

            chaser->children[tempPrev->val - 1] = newTreeNode ;
            head = chaser->children[tempPrev->val - 1] ;
        }
        tempPrev = temp;
        chaser = head ;

        head = head->children[temp->val-1] ;
        temp = temp->next ;
    }

    /*
    If the head is NULL, we need to create the DNFtreeNode into
    which the constraint will be inserted
    */
    if (head == NULL){
        DNFtreeNode * newTreeNode = malloc(sizeof(DNFtreeNode)) ;
        newTreeNode->scaled = false ;
        DNFtreeNode ** children = malloc(N*sizeof(DNFtreeNode *)) ;
        for (int i = 0 ; i < N ; i++){children[i] = NULL ;}
        
        newTreeNode->children = children ;

        DNFnode ** constraints = malloc(sizeof(DNFnode *)*N) ;
        for (int i = 0 ; i < N ; i++){constraints[i] = NULL ;}
        newTreeNode->constraints = constraints ;

        chaser->children[tempPrev->val - 1] = newTreeNode ;
    }
    chaser->children[tempPrev->val - 1]->constraints[tiles - 1] = constraint ;
    return ;
}

void scaleFullLength(DNFtreeNode * treeNode){
    if (treeNode->scaled == false){
        DNFnode * temp = treeNode->constraints[N-1] ;
        while (temp != NULL){
            for (int i = 0 ; i < N ; i++){
                if (temp->term[i] > 0){
                    temp->term[i] = i+1 ;
                } else {
                    temp->term[i] = -1-i ;
                }
            }
            temp = temp->next ;
        }
        treeNode->scaled = true ;
    }
    for (int child = 0 ; child < N ; child++){
        if (treeNode->children[child] != NULL){
            scaleFullLength(treeNode->children[child]) ;
        }
    }
    return ;

}

void printNodeRec(node * description, DNFtreeNode * treeNode){
    // Print the description
    node * temp = description ;
    printf("**************\n") ;
    printf("Description:\t< ") ;
    while (temp != NULL){
        printf("%d ",temp->val) ;
        temp = temp->next ;
    }
    printf(">\tTiles: %d\n",N) ;
    printf("\n") ;
    DNFnode *tempC = treeNode->constraints[N-1] ;
    while (tempC != NULL){
        printf("\t<") ;
        for (int j = 0 ; j < N ; j++){
            printf("%d ",tempC->term[j]) ;
        }
        printf(">\n") ;
        tempC = tempC->next ;
    }

    for (int ch = 0 ; ch < N ; ch++){
        if (treeNode->children[ch] != NULL){
            node * temp = malloc(sizeof(node)) ;
            temp = description ;
            node * tempAdd = malloc(sizeof(node));
            tempAdd->val = ch+1 ;
            tempAdd->tail = tempAdd ;
            temp->tail->next = tempAdd ;
            temp->tail = tempAdd ;
            printNodeRec(temp,treeNode->children[ch]) ;
        }
    }

    return ;
}

bool inCNFtree(node * description, CNFtreeNode * root){
    node * temp = description ;
    CNFtreeNode * head = root ;

    /* 
    Move through the tree to the node corresponding to the description
    that we care about. Check that the new head is not NULL each
    time to make sure we don't get a seg fault.
    */
    while (temp != NULL){
        if (head == NULL){
            return false ;
        }
        head = head->children[temp->val - 1] ;
        temp = temp->next ;
    }
    /* 
    After the loop we want to see if the CNF formula has
    been created for the description. First we have to check that the
    node we iterated to is not NULL, as trying to access its cnf field
    will result in a seg fault if so.
    */
    if (head == NULL){
        return false ;
    }
    // Now we can finally check if the CNF has been created yet
    return head->cnf != NULL ;
}

void insertCNF(node * description, CNFnode * cnf, CNFtreeNode * root){
    // Traverse the tree
    node * temp = description ;
    node * tempPrev = NULL ;
    CNFtreeNode * head = root ;
    CNFtreeNode * chaser = NULL ;
    

    while (temp != NULL){
        if (head == NULL){
            CNFtreeNode * newTreeNode = malloc(sizeof(CNFtreeNode)) ;
            CNFtreeNode ** children = malloc(N*sizeof(CNFtreeNode *)) ;
            for (int i = 0 ; i < N ; i++){children[i] = NULL ;}
            newTreeNode->children = children ;

            chaser->children[tempPrev->val - 1] = newTreeNode ;
            head = chaser->children[tempPrev->val - 1] ;
        }
        tempPrev = temp;
        chaser = head ;

        head = head->children[temp->val-1] ;
        temp = temp->next ;
    }

    /*
    If the head is NULL, we need to create the DNFtreeNode into
    which the constraint will be inserted
    */
    if (head == NULL){
        CNFtreeNode * newTreeNode = malloc(sizeof(CNFtreeNode)) ;
        CNFtreeNode ** children = malloc(N*sizeof(CNFtreeNode *)) ;
        for (int i = 0 ; i < N ; i++){children[i] = NULL ;}
        
        newTreeNode->children = children ;

        chaser->children[tempPrev->val - 1] = newTreeNode ;
    }
    chaser->children[tempPrev->val - 1]->cnf = cnf ;
    return ;
}

struct CNFnode * explode(DNFnode * dnf, int * accumulator){
    CNFnode * ret = NULL ;
    // Clauses might not set values for all variables so we have to initialize to zero so empty spots are zeros

    for (int i = 0 ; i < N ; i++){
        if (dnf->term[i] != -1*accumulator[i]){
            CNFnode * t = malloc(sizeof(CNFnode)) ;
            int * indicator = malloc(N*sizeof(int)) ;
            // Clauses might not set values for all variables so we have to initialize to zero so empty spots are zeros
            for (int j = 0 ; j < N ; j++){indicator[j] = 0 ;} 
            t->tail = t ;
            t->clause = indicator ;
            t->clause[i] = dnf->term[i] ;
            ret = mergeCNF(ret,t) ;
        }
        
    }
    return ret ;
}

CNFnode * f(DNFnode * dnf, int * accumulator){
    if (dnf == NULL){
        return NULL ;
    }
    if (dnf->next == NULL){
        return explode(dnf,accumulator) ;
    }
    CNFnode * ret = NULL ;
    for (int j = 0 ; j < N ; j++){
        if (dnf->term[j] == -1*accumulator[j]){
            continue ;
        } else {
            CNFnode * rec = malloc(sizeof(CNFnode)) ;
            int * newAcc = malloc(N*sizeof(int)) ;
            memcpy(newAcc, accumulator, N*sizeof(int));
            newAcc[j] = dnf->term[j] ;
            rec = f(dnf->next, newAcc) ;
            ret = mergeCNF(ret,incorporate(dnf->term[j],j,rec)) ;
        }
    }
    return ret ;
}

CNFnode * incorporate(int literal, int index, CNFnode * cnf){
    CNFnode * p = cnf ;
    while (p != NULL){
        p->clause[index] = literal ;
        p = p->next ;
    }
    return cnf ;
}

CNFnode * mergeCNF(CNFnode * first, CNFnode * second){
    if (first == NULL){
        return second ;
    } else if (second == NULL){
        return first ;
    } else {
        first->tail->next = second ;
        first->tail = second->tail ;
        return first ;
    }
}

bool isSubConstraint(CNFnode * sub, CNFnode * super){
    for (int i = 0 ; i < N ; i++){
        if (sub->clause[i] != 0 && sub->clause[i] != super->clause[i]){
            return false ;
        }
    }
    return true ;
}

CNFnode * removeRedundant(CNFnode * cnf){
    CNFnode * sub = cnf ;
    while (sub != NULL){
        CNFnode * super = cnf ;
        CNFnode * superPrev = NULL ;
        while (super != NULL){
            // Delete Time!
            if (sub != super && isSubConstraint(sub,super)){
                // Deleting the first constraint
                if (cnf == super){
                    cnf = super->next ;
                    free(super->clause) ;
                    free(super) ;
                    super = cnf ;
                } else { // Deleting a middle clause
                    superPrev->next = super->next ;
                    free(super->clause) ;
                    free(super) ;
                    super = superPrev->next ;
                }
            } else {
                superPrev = super ;
                super = super->next ;
            }
        }
        sub = sub->next ;
    }
    return cnf ;
}

CNFnode * setLength(CNFnode * formula){
    CNFnode * temp = formula ;
    while (temp != NULL){
        int len = 0 ;
        for (int i = 0 ; i < N ; i++){
            if (temp->clause[i] != 0){
                len = len + 1 ;
            }
        }
        temp->len = len ;
        temp = temp->next ;
    }
    return formula ;
}

void printCNF(node * description, CNFtreeNode * treeNode){
    // Walk through the tree
    struct node * temp = description ;
    struct CNFtreeNode * head = treeNode ;
    while (temp != NULL){
        head = head->children[temp->val - 1] ;
        temp = temp->next ;
    }
    // Print the description
    node * tempPrint = description ;
    printf("**************\n") ;
    printf("Description:\t< ") ;
    while (tempPrint != NULL){
        printf("%d ",tempPrint->val) ;
        tempPrint = tempPrint->next ;
    }
    printf(">\tTiles: %d\n",N) ;
    printf("\n") ;
    CNFnode *tempC = head->cnf ;
    while (tempC != NULL){
        printf("\t<") ;
        for (int j = 0 ; j < N ; j++){
            printf("%d ",tempC->clause[j]) ;
        }
        printf(">\n") ;
        tempC = tempC->next ;
    }

    return ;
}

CNFnode * removeEmpty(CNFnode * cnf){
    CNFnode * temp = cnf ;
    CNFnode * tempPrev = NULL ;
    while (temp != NULL){
        if (emptyClause(temp)){ // We're deleting!
            if (cnf == temp){ // The node to delete is the first node in the linkages
                cnf = temp->next ;
                free(temp->clause) ;
                free(temp) ;
                temp = cnf ;
            } else { // The node to delete is in the middle of the linkages (or the final one)
                tempPrev->next = temp->next ;
                free(temp->clause) ;
                free(temp) ;
                temp = temp->next ;
            }
        } else {
            tempPrev = temp ;
            temp = temp->next ;
        }
    }
    return cnf ;
}

bool emptyClause(CNFnode * cnf){
    for (int i = 0 ; i < N ; i++){
        if (cnf->clause[i] != 0){
            return false ;
        }
    }
    return true ;
}
/*
CNFnode * subsumption(CNFnode * cnf, int clauseLength){
    CNFnode * subsumer = cnf ;
    
    while (subsumer != NULL){
        CNFnode * subsumed = cnf ;
        CNFnode * subsumedPrev = NULL ;

        while (subsumed != NULL){
            if (subsumer != subsumed && canSubsume(subsumer,subsumed, clauseLength)){
                if (subsumed == cnf){ // we're deleting the first node of the linkages
                    cnf = subsumed->next ;
                    free(subsumed->clause) ;
                    free(subsumed) ;
                    subsumed = cnf ;
                } else {
                    subsumedPrev->next = subsumed->next ;
                    free(subsumed->clause) ;
                    free(subsumed) ;
                    subsumed = subsumedPrev->next ;
                }
            } else {
                subsumedPrev = subsumed ;
                subsumed = subsumed->next ;
            }
        }
        subsumer = subsumer->next ;
    }
    return cnf ;
} */

CNFnode * subsumption(CNFnode * cnf, int clauseLength){
    CNFnode * subsumer = cnf ; // This one eats

    while (subsumer != NULL){ // Everyone gets a chance to eat
        CNFnode * subsumed = cnf ; // This one gets eaten
        CNFnode * subsumedPrev = NULL ;
        while (subsumed != NULL){ // Everyone can get eaten
            if (subsumer != subsumed){ // Can't eat yourself!
                if (canSubsume(subsumer, subsumed, clauseLength)){ // Time to eat!
                    if (subsumed == cnf){ // Need to be careful eating the head, lest everything is lost
                        cnf = cnf->next ; // cnf takes subsumed out of the loop
                        free(subsumed->clause) ; // Eat the malloc-ed fields of the struct first
                        free(subsumed) ; // Then the struct
                        subsumed = cnf ; // Back in the loop
                    } else { // Eat a link in the middle
                        subsumedPrev->next = subsumed->next ; // subsumedPrev takes subsumed out of the loop
                        if (cnf->tail == subsumed){ // Need to consider if eating the tail, reassigning it if so
                            cnf->tail = subsumedPrev ; // cnf changes tail field to reflect the tail-to-be
                        }
                        free(subsumed->clause) ; // Eat the malloc-ed fields of the struct first
                        free(subsumed) ; // Then the struct
                        subsumed = subsumedPrev->next ; // Back in the loop (will be NULL if tail was eaten)
                    }
                } else { // No eating (this time...)
                    subsumedPrev = subsumed ; // Move subsumedPrev up
                    subsumed = subsumed->next ; // Move subsumed up
                }
            } else { // Got to wait until the next iteration to consider eating
                subsumedPrev = subsumed ; // Move subsumedPrev up
                subsumed = subsumed->next ;// Move subsumed up
            }
        }
        subsumer = subsumer->next ; // This clause has considered every possible clause to eat, so now the next gets the chance
    }
    return cnf ; // Return the cnf after eating as much of it as subsumption allows
}
bool canSubsume(CNFnode * subsumer, CNFnode * subsumed, int clauseLength){
    for (int i = 0 ; i < clauseLength ; i++){
        if (subsumer->clause[i] != 0 && subsumer->clause[i] != subsumed->clause[i]){
            return false ;
        }
    }
    return true ;
}

int * scaleRow(int * scaleFrom, int * fill, int index){
    for (int i = 0 ; i < N ; i++){
        if (scaleFrom[i] > 0){
            fill[i] = scaleFrom[i] + index*N ;
        } else if (scaleFrom[i] < 0){
            fill[i] = scaleFrom[i] - index*N ;
        } else {
            continue ;
        }
    }
    return fill ;
}
int * scaleColumn(int * scaleFrom, int * fill, int index){
    for (int i = 0 ; i < N ; i++){
        if (scaleFrom[i] > 0){
            fill[i] = i*N + index + 1 ;
        } else if (scaleFrom[i] < 0){
            fill[i] = -1 * (i*N + index) - 1 ;
        } else {
            continue ;
        }
    }
    return fill ;
}

CNFnode * copyCNFscaled(node * description, CNFtreeNode * root, int index, char line){
    CNFtreeNode * tempRoot = root ;
    if (description->val != 0){
        node * tempDescription = description ;
        while (tempDescription != NULL){
            tempRoot = tempRoot->children[tempDescription->val - 1] ;
            tempDescription  = tempDescription->next ;
        }
    }

    CNFnode * toCopy = tempRoot->cnf ;

    CNFnode * copyRoot = malloc(sizeof(CNFnode)) ;

    copyRoot->tail = copyRoot ;
    copyRoot->len = toCopy->len ;
    int * clause = malloc(N*N*sizeof(int)) ;
    copySmallToBig(toCopy->clause,clause,index,line) ;
    copyRoot->clause = clause ;

    CNFnode * prev = copyRoot ;
    CNFnode * curr = toCopy->next ;

    while (curr != NULL){
        CNFnode * newNode = malloc(sizeof(CNFnode)) ;
        newNode->tail = newNode ;
        newNode->len = curr->len ;
        prev->next = newNode ;
        copyRoot->tail = newNode ;

        int * newClause = malloc(N*N*sizeof(int)) ;
        copySmallToBig(curr->clause,newClause,index,line) ;
        newNode->clause = newClause ;

        prev = prev->next ;
        curr = curr->next ;
    }

    return copyRoot ;
}

void copySmallToBig(int * small, int * big, int index, char line){
    if (line == 'r'){
        for (int i = 0 ; i < N ; i++){
            if (small[i] > 0){
                big[index*N+i] = small[i] + index*N ;
            } else if (small[i] < 0){
                big[index*N+i] = small[i] - index*N ;
            } else {}
        }
    } else {
        for (int i = 0 ; i < N ; i++){
            if (small[i] > 0){
                big[i*N+index] = i*N + index + 1 ;
            } else if (small[i] < 0){
                big[i*N+index] = -1 * (i*N + index) - 1 ;
            } else {}
        }
    }
    return ;
}

/*
CNFnode * copyCNFscaled(node * description, CNFtreeNode * root, int index, char line){
    // Traverse the tree to the proper node (only when not working with empty description)
    CNFtreeNode * tempRoot = root ;
    if (description->val != 0){
        node * tempDescription = description ;
        while (tempDescription != NULL){
            tempRoot = tempRoot->children[tempDescription->val - 1] ;
            tempDescription = tempDescription->next ;
        }
    }
    

    // Start copying over
    CNFnode * toCopy = tempRoot->cnf ;

    CNFnode * rootCopy = malloc(sizeof(CNFnode)) ;
    rootCopy->tail = rootCopy ;
    rootCopy->len = toCopy->len ;

    int * clause = malloc(sizeof(int)*N*N) ;
    for (int i = 0 ; i < N ; i++){
        clause[toCopy->clause[i]-1] = toCopy->clause[i] ;
    }
    rootCopy->clause = clause ;
    
    if (line == 'r'){
        clause = scaleRow(toCopy->clause, clause, index) ;
    } else {
        clause = scaleColumn(toCopy->clause,clause,index) ;
    } 
    

    CNFnode * prev = rootCopy ;
    CNFnode * curr = toCopy->next ;

    // Copy the rest of the clauses over
    while (curr != NULL){
        CNFnode * new = malloc(sizeof(CNFnode)) ;
        new->tail = new ;
        prev->next = new ;
        rootCopy->tail = new ;
        new->len = curr->len ;

        int * clause = malloc(N*N*sizeof(int)) ;
        for (int i = 0 ; i < N ; i++){
            clause[curr->clause[i]-1] = curr->clause[i] ;
        }
        if (line == 'r'){
            clause = scaleRow(curr->clause,clause,index) ;
        } else {
            clause = scaleColumn(curr->clause,clause,index) ;
        } 
        new->clause = clause ;
        
        prev = prev->next ;
        curr = curr->next ;
    }

    CNFnode * backThrough = rootCopy ;
    while (backThrough != NULL){
        if (line == 'r'){
            for (int i = index*N ; i < (index+1)*N ; i++){
                if (backThrough->clause[i] > 0){
                    backThrough->clause[i] = backThrough->clause[i] + index*N ;
                } else if (backThrough->clause[i] < 0) {
                    backThrough->clause[i] = backThrough->clause[i] - index*N ;
                } else {}
            }
        } else {
            for (int i = index*N ; i < (index+1)*N ; i++){
                if (backThrough->clause[i] > 0){
                    backThrough->clause[i] = i*N + index + 1 ;
                } else if (backThrough->clause[i] < 0){
                    backThrough->clause[i] = -1 * (i*N + index) - 1 ;
                } else {}
            }
        }
        backThrough = backThrough->next ;
    }
    return rootCopy ;
} */


// emptyLineCNF doesn't need to change. It's the scaling and cleaning process.
CNFnode * emptyLineCNF(){
    CNFnode * root = malloc(sizeof(CNFnode)) ;
    root->len = 1 ;
    root->tail = root ;
   

    int * clause = malloc(N*sizeof(int)) ;
    clause[0] = -1 ;
    for (int i = 1 ; i < N ; i++){clause[i] = 0 ;}

    root->clause = clause ;

    CNFnode * prev = root ;

    for (int i = 1 ; i < N ; i++){
        CNFnode * newClause = malloc(sizeof(CNFnode)) ;

        prev->next = newClause ;
        root->tail = newClause ;
        newClause->len = 1 ;
        int * clause = malloc(N*sizeof(int)) ;

        for (int j = 0 ; j < N ; j++){
            if (i == j){
                clause[j] = -1 * (j+1) ;
            } else {
                clause[j] = 0 ;
            }
        }

        newClause->clause = clause ;
        prev = newClause ;
    }
    return root ;
}


/*

    DNFnode * example2 ;
    int term21[N] = {1,-2,-3} ;
    example2->term = term21 ;
    
    DNFnode * example2Term2 ;
    int term22[N] = {-1,2,-3} ;
    example2Term2->term = term22 ;

    DNFnode * example2Term3 ;
    int term23[N] = {-1,-2,3} ;
    example2Term3->term = term23 ;

    example2->next = example2Term2 ;
    example2Term2->next = example2Term3 ;
    example2->tail = example2Term3 ;
    example2Term2->tail = example2Term3 ;
    example2Term3->tail = example2Term3 ;

    printDNF(example2) ;


    int * terms2 = malloc(sizeof(int)) ;
    *terms2 = 0 ;
    literalNode * frequencies2 = getFrequencies(example2,terms2) ;
    printf("Terms: %d\n", *terms2) ;
    for (int i = 0 ; i < 2*N ; i++){
        printLiteral(frequencies2[i]) ;
    }

    free(frequencies2) ;
    free(terms2) ;

    return 0 ;
*/

CNFnode * DNFtoCNF(DNFnode * dnf) {
    int * terms = malloc(sizeof(int)) ;
    *terms = 0 ;
    literalNode * frequencies = getFrequencies(dnf,terms) ;

    CNFnode * ledger = malloc(sizeof(CNFnode)) ;
    ledger->tail = NULL ; // While tail is NULL, there are no clauses in the ledger


    for (int i = 0 ; i < 2*N ; i++){
        printf("Literal: %d\tFrequency: %d\n", frequencies[i].literal, frequencies[i].frequency) ;
        literalNode literal = frequencies[i] ;
        if (literal.frequency == 0){
            break ; // If I start seeing non-appearing literals, none of them make clauses so I can break
        }
        int potentialClause[N] ; // Might need to malloc and free this
        for (int i = 0 ; i < N ; i++){potentialClause[i] = 0 ;}
        potentialClause[abs(literal.literal) - 1] = literal.literal ;
        if (literal.frequency == *terms){ // Unit clauses
            addToLedger(potentialClause,ledger) ;
            continue ;
        }
        
        
        DNFnode * freeTerms[*terms-literal.frequency] ;// = malloc((N-literal.frequency)*sizeof(DNFnode *)) ;
        int pointerToLeave = 0 ;
        DNFnode * temp = dnf ;
        while (temp != NULL){
            if (temp->term[abs(literal.literal)-1] != literal.literal){
                freeTerms[pointerToLeave] = temp ;
                pointerToLeave += 1 ;
            }
            temp = temp->next ;
        }
        printf("\n\nLiteral: %d\tFrequency: %d\tIntended Free Terms: %d\n", literal.literal,literal.frequency,*terms-literal.frequency) ;
        for (int i = 0 ; i < *terms-literal.frequency ; i++){ // Prints the free terms
            printf("< ") ;
            for (int j = 0 ; j < N ; j++){
                printf("%d ",freeTerms[i]->term[j]) ;
            }
            printf(">\n") ;
        }
        
        int indices[*terms-literal.frequency] ;
        for (int i = 0 ; i < *terms-literal.frequency ; i++){indices[i] = 0 ;}
        bool incremented = true ;
        bool tautological = false ;
        bool subsumed = false ;
        int finalTerm = 0 ;
        int clausesToConsider = 50 ;
        while (incremented && clausesToConsider > 0){
            printf("potentialClause at the beginning of the loop: < ") ;
            for (int c = 0 ; c < N ; c++){
                printf("%d ",potentialClause[c]) ;
            }
            printf(">\n") ;
            incremented = false ;
            tautological = false ;
            subsumed = false ;
            finalTerm = 0 ;
            
            for (int j = 0 ; j < *terms-literal.frequency ; j++){
                finalTerm = j ;
                if (freeTerms[j]->term[indices[j]] == -1*potentialClause[indices[j]]){
                    tautological = true ;
                    printf("Tautology: %d\n", freeTerms[j]->term[indices[j]]) ;
                    break ;
                } else {
                    potentialClause[indices[j]] = freeTerms[j]->term[indices[j]] ;
                }
                
                if (ledgerSubsumes(potentialClause,ledger)){
                    subsumed = true ;
                    printf("Subsumed:\t") ;
                    printf("< ") ;
                    for (int i = 0 ; i < N ; i++){
                        printf("%d ",potentialClause[i]) ;
                    }
                    printf(">\n") ;
                    break ;
                }
                
            }

            if (!subsumed && !tautological){
                printf("Adding this clause: (literal %d)\t< ",literal.literal) ;
                for (int i = 0 ; i < N ; i++){
                    printf("%d ",potentialClause[i]) ;
                }
                printf(">\n") ;
                addToLedger(potentialClause,ledger) ;
            }
            
            for (int i = finalTerm ; i > -1 ; i--){
                if (indices[i] == N-1){
                    indices[i] = 0 ;
                } else {
                    indices[i] += 1 ;
                    incremented = true ;
                    break ;
                }
            }
            
            for (int i = finalTerm+1 ; i < *terms-literal.frequency ; i++){
                indices[i] = 0 ;
            }
            
            if (incremented){
                for (int i = 0 ; i < N ; i++){
                    if (i != abs(literal.literal) - 1){
                        potentialClause[i] = 0 ;
                    }
                }
            }
            printf("finalTerm: %d\tindices: [", finalTerm) ;
            for (int i = 0 ; i < *terms-literal.frequency ; i++){
                printf("%d,", indices[i]) ;
            }
            printf("]\n") ;
            clausesToConsider -= 1 ;
            
        } 
        break ;
        
    }

    free(frequencies) ;
    free(terms) ;

    return ledger ;
}

literalNode * getFrequencies(DNFnode * dnf, int * termCount){
    literalNode * frequencies = malloc(2*N*sizeof(literalNode)) ;
    // Initialize with zero counts
    for (int i = 0 ; i < 2*N; i++){
        literalNode literal ;
        literal.frequency = 0 ;
        if (i > N - 1){
            literal.literal = N-i-1 ;
        } else {
            literal.literal = i + 1 ;
        }
        frequencies[i] = literal ;
    }
    // Build up the counts of each literal in the DNF formula (also accumulate number of terms)
    DNFnode * temp = dnf ;
    while (temp != NULL){
        *termCount += 1 ;
        for (int j = 0 ; j < N ; j++){
            if (temp->term[j] < 0){
                frequencies[N+j].frequency += 1 ;
            } else {
                frequencies[j].frequency += 1 ;
            }
        }
        temp = temp->next ;
    }

    // Sort by frequency in descending order
    for (int i = 1 ; i < 2*N ; i++){
        literalNode temp = frequencies[i] ;
        int j = i-1 ;

        while (j >= 0 && temp.frequency > frequencies[j].frequency){
            frequencies[j+1] = frequencies[j] ;
            j -= 1 ;
        }
        frequencies[j+1] = temp ;
    }


    return frequencies ;
}

bool ledgerSubsumes(int * potentialClause, CNFnode * ledger){
    if (ledger->tail == NULL){
        return false ;
    }
    CNFnode * temp = ledger ;
    while (temp != NULL){
        if (isSubsumed(potentialClause,temp)){
            return true ;
        }
        temp = temp->next ;
    }
    return false ;
}

bool isSubsumed(int * subsumed, CNFnode * subsumer){
    literalIndexNode * literalIndices = subsumer->indices ;
    while (literalIndices != NULL){
        if (subsumer->clause[literalIndices->index] != subsumed[literalIndices->index]){
            return false ;
        }
        literalIndices = literalIndices->next ;
    }
    return true ;
}

void addToLedger(int * clause, CNFnode * ledger){
    if (ledger->tail == NULL){
        int * cnf = malloc(N*sizeof(int)) ;
        for (int i = 0 ; i < N ; i++){
            cnf[i] = clause[i] ;
            if (clause[i] != 0){
                literalIndexNode * index = malloc(sizeof(literalIndexNode)) ;
                index->index = i ;
                if (ledger->indices == NULL){
                    ledger->indices = index ;
                    ledger->indices->tail = ledger->indices ;
                } else {
                    ledger->indices->tail->next = index ;
                    ledger->indices->tail = index ;
                }
            }
        }
        ledger->tail = ledger ;
        ledger->clause = cnf ;
    } else {
        CNFnode * cnf = malloc(sizeof(CNFnode)) ;
        int * c = malloc(N*sizeof(int)) ;
        for (int i = 0 ; i < N ; i++){
            c[i] = clause[i] ;
            if (clause[i] != 0){
                literalIndexNode * index = malloc(sizeof(literalIndexNode)) ;
                index->index = i ;
                if (cnf->indices == NULL){
                    cnf->indices = index ;
                    cnf->indices->tail = cnf->indices ;
                } else {
                    cnf->indices->tail->next = index ;
                    cnf->indices->tail = index ;
                }
            }
        }
        cnf->clause = c ;
        ledger->tail->next = cnf ;
        ledger->tail = cnf ;
    }
    return ;
}