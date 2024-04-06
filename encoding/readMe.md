# Chapter 2 -- Encoding Nonogram in CNF

When encoding, I made use of a Mersenne twister algorithm for generating random numbers to fill the boards randomly. The algorithm was written by Evan Sultanik, and it can be found [here](https://github.com/ESultanik/mtwister). Additionally, to avoid reading and writing to file repeatedly in the board generating process, a buffer structure was used and occasionally dumped to file. The buffer implementation was written by Alcover and can be found [here](https://github.com/alcover/buf) (*really* nicely written documentation).
