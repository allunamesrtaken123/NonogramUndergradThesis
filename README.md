# NonogramUndergradThesis

This repository is the companion to my undergraduate thesis, which can be found [here](https://digitalcollections.wesleyan.edu/islandora/complexity-and-threshold-behavior-playing-nonogram-puzzles). All of the final versions of the files used for the thesis are included, organized by the chapter in which the code or its output is discussed. Additionally, the LaTeX code necessary to compile my thesis is included. 

If you have any questions, comments, or concerns, reach out to me at [afoote@wesleyan.edu](mailto:afoote@wesleyan.edu?subject=NonogramUndergradThesisRepo).


## LaTeX

#### Getting it Running
The template I used for my thesis is based on the [one used by Diego Calderon](https://github.com/dcalderon/senior-project-es2), which builds on [the template provided by Professor Norman Danner](https://www.wesleyan.edu/mathcs/cs/lab_resources.html). It is modular, such that you have one file in which all packages are imported, and then each chapter is included as its own file. For my thesis, the main file it named `head.tex`. I developed it using TeXShop on my local device. To get all of the links and citations working you have to typeset `head.tex` multiple times (using the button in the top left corner). You don't typeset any other document. It's not an exact science for me, but I always start by typsetting using LaTeX. At that point the figures show and they might be linked, but the citations usually haven't populated yet. Next I typeset using BibTeX. That fixes the citations, replacing the question marks with the formatted citations. Finally, to get everything linked properly, I usually have to compile once or twice more using LaTeX. 

#### Drawing Puzzles
At the start of the project I used [logicpuzzle.sty](https://ctan.math.utah.edu/ctan/tex-archive/graphics/pgf/contrib/logicpuzzle/logicpuzzle.pdf) to draw the puzzles. It's a wonderful package that can draw pretty clean puzzles. However, nothing beats TikZ for drawing in LaTeX. All of the gadgets in chapter three are drawn using TikZ. The TikZ package is needed to annotate the boards with variable names, as far as I can tell. It is probably more difficult to draw Minesweeper boards using TikZ than Sudoku or Nonogram, but [this package](https://github.com/T0nyX1ang/tikz-minesweeper) might be useful for more drawing more involved Minesweeper boards (I didn't test it at all though).
