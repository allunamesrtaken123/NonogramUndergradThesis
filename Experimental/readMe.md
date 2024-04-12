# Chapter 4 -- Experimental Results


## Scraped Puzzles

The scraped puzzles analyzed in the thesis come from the website [https://www.nonograms.org/nonograms](https://www.nonograms.org/nonograms). Puzzles published after 2/4/24 are not included in the analysis. The process involves scraping the data for the puzzle and storing it in a JSON file. For an example, [puzzle 50196](https://www.nonograms.org/nonograms/i/50196) would be stored as:

```yaml
{
  "rows" : [[2],[1],[3],[1]],
  "columns" : [[1],[2],[3],[1]],
  "rowCount" : 4,
  "columnCount : 4
}
```

This scraping is accomplished with the script `puzzleScraping.py`. Next is to parse the puzzles into CNF. The script for this is `parseScrapedPuzzles.c`. This script uses a buffer struct implemented by Alcover, the documentation for which can be found [here](https://github.com/alcover/buf). Additionally, I parse the JSON files in C using the [Jansson library](https://jansson.readthedocs.io/en/latest/index.html#). The files are parsed into CNF formulae using the same process as that used for the randomly generated puzzles in the phase transition analysis.

Once the CNF formulae have been generated, the solving process is pretty similar to that of the randomly generated puzzles. The only difference is that now the dimensions have to be read from the JSON file before solving rather than just being set at the top of the script. The solving is done using the script `solvingScrapedPuzzles.py`.

There is way too much data to put it on GitHub, but these scripts should allow you to recreate the analysis, possibly even extending it with more recent puzzles.

#### Scraping Puzzles
#### Parsing Scraped Puzzles
The C script `parseScrapedPuzzles.c` is used to read the JSON files and convert them to CNF formulae. To compile, use the command `gcc -o parsePuzzles parseScrapedPuzzles.c buf.c -ljansson`. The elements that may need to be changed are the directory paths in lines 130 and 211. You should be able to keep the files paths the same.
