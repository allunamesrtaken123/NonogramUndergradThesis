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
The scraping was run in a two stage process. Since the puzzles are listed on separate pages, the first step was to compile all of the puzzle links. This was done using `linkScraping.py`. The scraping package used is [Selenium](https://www.selenium.dev/). A useful tutorial on web scraping with Selenium can be found [here](https://www.scrapingbee.com/blog/selenium-python/). To use Selenium you need a Chrome driver (it doesn't have to be Chrome but that's what I use), which can be accessed [here](https://chromedriver.chromium.org/downloads). The only line that needs to be altered is line 19, which contains the path to which the text file with puzzle links will be written.

Once the links have been scraped and stored, the puzzles themselves can be scraped using `puzzleScraping.py` and `straightPuzzleProcessing.py`. The file `straightPuzzleProcessing.py` extracts basic characteristics of each puzzle, such as the number of rows and columns, the difficulty rating of the puzzle, and the number of cells to be inferred. The file `puzzleScraping.py` scrapes the actual puzzle descriptions, storing them in a JSON format that can be parsed into CNF.

The only things to change for the files are the file paths to read in the links and to write the JSON/CSV files.

#### Parsing Scraped Puzzles
The C script `parseScrapedPuzzles.c` is used to read the JSON files and convert them to CNF formulae. To compile, use the command `gcc -o parsePuzzles parseScrapedPuzzles.c buf.c -ljansson`. The elements that may need to be changed are the directory paths in lines 130 and 211. You should be able to keep the files paths the same.

#### Solving Scraped Puzzles
The final step in the process is actually solving the puzzles to determine inferability. For this, `solvingScrapedPuzzles.py` is used. The only things to change are again the file paths.
