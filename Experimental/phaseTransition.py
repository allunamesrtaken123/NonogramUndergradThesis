from pysat.formula import CNF # You need to download this (see readMe)
from pysat.solvers import Glucose42 # You need to download this (see readMe)
from tqdm import tqdm
from time import time

# The filled cell densities
probs = [
    '0.03','0.06','0.09','0.12','0.15',
    '0.18','0.21','0.24','0.27','0.30',
    '0.33','0.36','0.39','0.42','0.45',
    '0.48','0.51','0.54','0.57','0.60',
    ]
boards = 250 # Number of boards at each density
n = 25 # Dimenstion of each board

# These are for bookkeeping
conflicts = [] # we actually track propagations
clauses = []
alphas = []
averageTimes = []

for p in range(1,21): # There are 20 puzzle densities to process, and they are 1-indexed
    print(f"Density: {probs[p-1]}")
    for b in tqdm(range(boards)):
        t1 = time()
        f1 = CNF(from_file=f'/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/General-Inferability/{n}x{n}/{p} {b}.cnf') # change this file path
        with Glucose42(bootstrap_with = f1) as m:
            inferred = 0
            clauses.append(m.nof_clauses()) # Note the number of clauses
            """
            For each cell in the board, determine whether an inference is possible by assuming the cell is empty and testing
            the board for consistency under that assumption.
            """
            for i in range(1,n*n+1):
                if not m.solve(assumptions = [-i]):
                    inferred += 1 
            stats = m.accum_stats()
            conflicts.append(stats['propagations'])
            alphas.append(inferred)
        averageTimes.append(time() - t1)

# Bookkeep
with open(f"/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/General-Inferability/Phase-Transition-CSV/filledInference{n}x{n}.csv","w") as f: # change this file path
    f.write("density,board,alpha,conflicts,clauses,timeTaken\n")
    for i,p in enumerate(range(len(probs))):
        for j in range(boards):
            index = boards*i + j
            f.write(f'{probs[i]},{j},{alphas[index]},{conflicts[index]},{clauses[index]},{averageTimes[index]}\n')
f.close()

'''

I wanted to get the number of solutions but this is taking way too much time.

solutionCounts1 = []
#solutionCounts2 = []
for p in range(1,30):
    for b in tqdm(range(boards)):
        f1 = CNF(from_file = f'/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Generating-Puzzles/10x10/{p} {b}.cnf')
        with Glucose42(bootstrap_with = f1) as m:
            solutionCounts1.append(len(list(m.enum_models())))
with open(f'/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Generating-Puzzles/Testing/solutions.csv','w') as f:
    f.write('density,board,solutions\n')
    for i,p in enumerate(range(len(probs))):
        for j in range(boards):
            index = boards*i + j
            f.write(f'{probs[i]},{j},{solutionCounts1[index]}\n')
f.close()
'''