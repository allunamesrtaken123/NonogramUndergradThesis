library(tidyverse)
library(readr)
library(plotly)
library(ggplot2)
library(extrafont)
library(scales)
  
  # Stats from solving puzzles
scrapedPuzzles <- read_csv("/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/scrapedPuzzleData.csv")
  # Stats about the boards
puzzleStats <- read_csv("/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/scrapedNonogram.csv")

puzzleFull <- puzzleStats %>%
  left_join(scrapedPuzzles, by = c('index' = 'puzzleIndex')) %>%
  filter(!is.na(alpha)) %>%
  mutate(totalTiles = rows * columns) %>%
  mutate(tileDensity = tilesToInfer/totalTiles) %>%
  mutate(difficultyLevel = ifelse(difficulty <= 3,'easy',
                                  ifelse(difficulty >= 7,'hard','medium'))) %>%
  mutate(difficultyLevel = factor(difficultyLevel,levels = c('easy','medium','hard')),
         timePerTile = timeTaken/totalTiles,
         propagationsPerTile = propagations/totalTiles,
         propagationsPerClause = propagations/clauses,
         timePerClause = timeTaken/clauses)

easyPuzzles <- puzzleFull %>% filter(difficultyLevel == 'easy') %>%
  group_by(tileDensity) %>% 
  summarize(propagationsPerTile = mean(propagationsPerTile))

# Tables of Summary Statistics
ggplotly(puzzleFull %>% group_by(difficulty) %>%
  summarize(propagationsPerTile = mean(propagationsPerTile)) %>%
  ggplot(aes(x = difficulty, y = propagationsPerTile)) + geom_point())
puzzleFull %>% group_by(difficulty) %>%
  summarize(microsecondsPerTile = 1000000*mean(timePerTile)) %>%
  ggplot(aes(x = difficulty, y = millisecondsPerTile)) + geom_point()

puzzleFull %>% group_by(difficulty) %>%
  summarize(propagationsPerTile = mean(propagationsPerTile),
            microsecondsPerTile = 1000000*mean(timePerTile))

puzzleFull %>% group_by(difficultyLevel) %>%
  summarize(propagationsPerTile = mean(propagationsPerTile),
            microsecondsPerTile = 1000000*mean(timePerTile))

# chi-squared tests
  
forAOV <- puzzleFull %>% 
  mutate(difficulty = factor(difficulty,levels = c(2,3,4,5,6,7,8,9)))

propagationsAOVallLevel <- aov(propagationsPerTile ~ difficulty, data = forAOV) 
summary(propagationsAOVallLevel)
TukeyHSD(propagationsAOVallLevel)

propagationsAOVbinLevel <- aov(propagationsPerTile ~ difficultyLevel, data = forAOV) 
summary(propagationsAOVbinLevel)
TukeyHSD(propagationsAOVbinLevel)

timeTakenAOVallLevel <- aov(timePerTile ~ difficulty, data = forAOV) 
summary(timeTakenAOVallLevel)
TukeyHSD(timeTakenAOVallLevel)

timeTakenAOVbinLevel <- aov(timePerTile ~ difficultyLevel, data = forAOV) 
summary(timeTakenAOVbinLevel)
TukeyHSD(timeTakenAOVbinLevel)

# Size By Difficulty
puzzleFull %>% group_by(difficulty) %>% 
  summarize(avgSize = mean(totalTiles)) %>%
  pivot_wider(names_from = difficulty, values_from = avgSize) %>%
  cbind("Average Tiles") %>%
  rename("Difficulty" = `"Average Tiles"`) -> tileByDifficulty
tileByDifficulty[,c(9,1:8)]

# Bar Chart of Difficulty
puzzleFull %>%
  count(difficulty) %>%
  plot_ly(x = ~difficulty, y = ~n, type = 'bar')
ggplotly(ggplot(data = forAOV,aes(x = difficulty)) + geom_bar())

# Table of Sizes by Difficulty Level (is LaTeX table already)
puzzleFull %>% group_by(difficultyLevel) %>%
  summarize(avgSize = mean(totalTiles))


## Single Solution Analysis

solutionCounts <- read_csv("COURSES/Krizanc-Tutorials/Senior-Spring/Chapter-5/modelCountingActual.csv")
inference <- read_csv("COURSES/Krizanc-Tutorials/Senior-Spring/Chapter-5/inference.csv")

fullSolutions <- inner_join(solutionCounts,inference, by = c('density','board'))

fullSolutions %>% group_by(density) %>%
  summarize(propOneSolution = 1 - mean(multipleSolutions)) -> propOne

fullSolutions %>% group_by(density) %>%
  summarize(propOneSolution = 1 - mean(multipleSolutions)) %>%
  plot_ly(x = ~density, y = ~propOneSolution, 
          mode = "lines+markers", type = "scatter")
spline_int <- as.data.frame(spline(propOne$density, propOne$propOneSolution))
ggplotly(ggplot() + 
  geom_point(data = propOne,aes(x = density, y = propOneSolution), size = 2) +
  geom_line(data = spline_int, aes(x = x, y = y)))

fullSolutions %>% group_by(density) %>%
  summarize(propOneSolution = 1 - mean(multipleSolutions)) %>%
  ggplot(aes(x = density, y = propOneSolution)) + geom_point()


oneSolution <- fullSolutions %>% filter(multipleSolutions == 0)
multipleSolutions <- fullSolutions %>% filter(multipleSolutions == 1)

oneSolution %>% group_by(density) %>%
  summarize(avgPropagation = mean(conflicts))

multipleSolutions %>% group_by(density) %>%
  summarize(avgPropagation = mean(conflicts))

singleSolPScraped <- rbind(
  puzzleFull %>% 
    mutate(from = "Scraped") %>% 
    select(from,propagationsPerTile),
  oneSolution %>% 
    mutate(from = "Random",propagationsPerTile = conflicts/20/20) %>% 
    select(from,propagationsPerTile)
)

ggplot(data = singleSolPScraped, 
       aes(x = propagationsPerTile, color = from, fill = from, alpha = 0.5)) + 
  geom_density() + 
  geom_rug()
ggsave(filename = '/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/Chapter-5/difficulty.tiff',
       device='tiff', dpi=1200)

## Pretty Plots

  # 1. Tile Density Histogram of Scraped Puzzles
  #' I don't want a title. I don't want a grey background, and I definitely
  #' want a border. I think I'll ggplot it and then ggplotly that to save it
ggplotly(ggplot(data = puzzleFull, aes(x = tileDensity)) + 
  geom_histogram(fill = "dodgerblue3", color = "black") +
  theme(panel.grid.major = element_blank(), 
        panel.grid.minor = element_blank(),
        panel.background = element_blank(),
        panel.border = element_rect(colour = "black", fill=NA,linewidth = 2)) + 
  xlab("Filled Cell Density") + ylab("Frequency") + 
  theme(text = element_text(size = 30,family="Times New Roman")))

  
# 2. Proportion of Puzzles with One Solution
fullSolutions %>% group_by(density) %>%
  summarize(propOneSolution = 1 - mean(multipleSolutions)) -> propOne
spline_int <- as.data.frame(spline(propOne$density, propOne$propOneSolution))
ggplotly(ggplot() + 
  geom_point(data = propOne,aes(x = density, y = propOneSolution),color = 'dodgerblue3', size = 3) +
  geom_line(data = spline_int, aes(x = x, y = y),color = 'dodgerblue3') +
  theme(panel.grid.major = element_blank(), 
        panel.grid.minor = element_blank(),
        panel.background = element_blank(),
        panel.border = element_rect(colour = "black", fill=NA,linewidth = 2)) + 
  xlab("Filled Cell Density") + ylab("Proportion of Puzzles with One Solution") + 
  theme(text = element_text(size = 30,family="Times New Roman")))

  
# 3. Propagations Per Tile Distribution
ggplot(data = singleSolPScraped, 
       aes(x = propagationsPerTile, color = from, fill = from)) + 
  geom_density(alpha = 0.5) + 
  guides(color = F) + 
  labs(fill='Puzzle Source:') +
  geom_rug() +
  theme(panel.grid.major = element_blank(), 
        panel.grid.minor = element_blank(),
        panel.background = element_blank(),
        panel.border = element_rect(colour = "black", fill=NA,linewidth = 2)) +
  theme(legend.position = c(0.75, 0.75)) +
  theme(legend.background = element_rect(size=0.5, linetype="solid")) + 
  xlab("Difficulty Level") + ylab("Frequency") + 
  theme(text = element_text(size = 30,family="Times New Roman"),
        legend.text = element_text(size=40),
        legend.title = element_text(size = 50))
ggsave(filename = '/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/Thesis Visuals/Chapter 5/Pretty/difficulty.tiff',
       device='tiff', dpi=1200)
  
# 4. Bar Chart of Difficulty Levels

ggplotly(ggplot(data = forAOV,aes(x = difficulty)) + 
  geom_bar(fill = "dodgerblue3",color = 'black',size = 1.5) +
  theme(panel.grid.major = element_blank(), 
        panel.grid.minor = element_blank(),
        panel.background = element_blank(),
        panel.border = element_rect(colour = "black", fill=NA,linewidth = 2)) + 
  xlab("Difficulty Level") + ylab("Frequency") + 
  theme(text = element_text(size = 30,family="Times New Roman")))


fifteenByFifteen <- read_csv("COURSES/Krizanc-Tutorials/Senior-Spring/General-Inferability/Phase-Transition-CSV/filledInference15x15.csv")
twentyByTwenty <- read_csv("COURSES/Krizanc-Tutorials/Senior-Spring/phaseTransitionResults/filledInference20x20.csv")
twenty5ByTwenty5 <- read_csv("COURSES/Krizanc-Tutorials/Senior-Spring/General-Inferability/Phase-Transition-CSV/filledInference25x25.csv")

fifteenByFifteen %>% 
  mutate(propInferred = alpha/15/15/density) %>%
  group_by(density) %>%
  summarize(averageAlpha = mean(propInferred),
            averagePropagation = mean(conflicts),
            averageTime = mean(timeTaken)) %>%
  mutate(size = "15x15") -> fifteenByFifteenAgg
twentyByTwenty %>% 
  mutate(propInferred = alpha/20/20/density) %>% 
  group_by(density) %>% 
  summarize(averageAlpha = mean(propInferred),
            averagePropagation = mean(propagations),
            averageTime = mean(timeTaken)) %>%
  mutate(size = '20x20') -> twentyByTwentyAgg
twenty5ByTwenty5 %>% 
  mutate(propInferred = alpha/25/25/density) %>% 
  group_by(density) %>% 
  summarize(averageAlpha = mean(propInferred),
            averagePropagation = mean(conflicts),
            averageTime = mean(timeTaken)) %>%
  mutate(size = '25x25') -> twenty5ByTwenty5Agg

maxProp20x20 <- max(twentyByTwentyAgg$averagePropagation)
maxProp15x15 <- max(fifteenByFifteenAgg$averagePropagation)
maxProp25x25 <- max(twenty5ByTwenty5Agg$averagePropagation)


twentyByTwentyAgg$propMaxPropagation <- twentyByTwentyAgg$averagePropagation/maxProp20x20
fifteenByFifteenAgg$propMaxPropagation <- fifteenByFifteenAgg$averagePropagation/maxProp15x15
twenty5ByTwenty5Agg$propMaxPropagation <- twenty5ByTwenty5Agg$averagePropagation/maxProp25x25

fifteenByFifteenAgg$symbol <- 'a'
twentyByTwentyAgg$symbol <- 'b'
twenty5ByTwenty5Agg$symbol <- 'c'

fig <- plot_ly(x = ~density, y = ~averageAlpha, 
        symbol = ~symbol, symbols = c('circle', 'square','diamond')) %>%
  add_trace(data = fifteenByFifteenAgg, type = "scatter", mode = "lines+markers",
            name = "15x15", marker = list(size = 20, color = 'black'),
            line = list(color = 'black')) %>%
  add_trace(data = twentyByTwentyAgg, type = "scatter", mode = "lines+markers",
            name = "20x20", marker = list(size = 20, color = 'black'),
            line = list(color = 'black')) %>%
  add_trace(data = twenty5ByTwenty5Agg, type = "scatter", mode = "lines+markers",
            name = "25x25", marker = list(size = 20, color = 'black'),
            line = list(color = 'black')) %>%
  add_trace(data = twentyByTwentyAgg, x = ~density, y = ~propMaxPropagation, 
            yaxis = "y2", mode = "lines+markers", type = "scatter",
            showlegend = F, marker = list(size = 20, color = 'green'),
            line = list(color = 'green')) %>%
  add_trace(data = fifteenByFifteenAgg, x = ~density, y = ~propMaxPropagation, 
            yaxis = "y2", mode = "lines+markers", type = "scatter",
            showlegend = F, marker = list(size = 20, color = 'green'),
            line = list(color = 'green')) %>%
  add_trace(data = twenty5ByTwenty5Agg, x = ~density, y = ~propMaxPropagation, 
            yaxis = "y2", mode = "lines+markers", type = "scatter",
            showlegend = F, marker = list(size = 20, color = 'green'),
            line = list(color = 'green')) %>%
  layout(xaxis = 
           list(title = list(text = "Filled Cell Density", 
                             font = list(size = 25,family='Times New Roman')),
                showgrid = F,
                tickfont = list(size = 22, family = 'Times New Roman'))) %>% 
  layout(yaxis = 
           list(title = list(text = "Proportion of Filled Cells Inferred",
                             font = list(size = 25,family='Times New Roman')),
                showgrid = F,
                tickfont = list(size = 22, family = 'Times New Roman'))) %>% 
  layout(legend = 
           list(x = 0.05, y = 0.9, font = list(size = 40),
                bordercolor = "black",borderwidth = 3)) %>%
  layout(yaxis2 = 
           list(overlaying = "y",
                side = "right",
                title = list(text = "Proportion of Maximum Propagations",
                             font = list(size = 25,family = 'Times New Roman', color = 'green'),
                             standoff = 0),
                showgrid = F,
                tickfont = list(size = 22, family = 'Times New Roman', color = 'green'))) %>%
  layout(margin = list(r = 100,l = 50,pad = -25))

fig


## Clause Count Plot
formulaSizeData <- read_csv("COURSES/Krizanc-Tutorials/Senior-Spring/Chapter-5/modelCounting.csv") %>%
  group_by(density) %>%
  summarize(avgClauses = mean(clauses),
            avgVariables = mean(variables))
plot_ly(data = formulaSizeData, type = "scatter", mode = "lines+markers") %>% 
  add_trace(x = ~density, y = ~avgClauses,
            marker = list(size = 20, color = 'dodgerblue3'),
            line = list(color = 'dodgerblue3')) %>%
  

ggplot() + 
  geom_point(data = formulaSizeData,
             aes(x = density, y = avgClauses), 
             size = 3,
             color = 'dodgerblue3') +
  geom_line(data = formulaSizeData, 
            aes(x = density, y = avgClauses),
            color = 'dodgerblue3') +
  theme(panel.grid.major = element_blank(), 
        panel.grid.minor = element_blank(),
        panel.background = element_blank(),
        panel.border = element_rect(colour = "black", fill=NA,linewidth = 2)) + 
  xlab("Filled Cell Density") + ylab("Avg. Clauses") + 
  theme(text = element_text(size = 30,family="Times New Roman")) + 
  scale_y_continuous(labels = label_comma())
ggsave(filename = '/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/Thesis Visuals/Chapter 5/Pretty/clauseCount.tiff',
       device='tiff', dpi=1200)

ggplot() + 
  geom_point(data = formulaSizeData,
             aes(x = density, y = avgVariables), 
             size = 3,
             color = 'dodgerblue3') +
  geom_line(data = formulaSizeData, 
            aes(x = density, y = avgVariables),
            color = 'dodgerblue3') +
  theme(panel.grid.major = element_blank(), 
        panel.grid.minor = element_blank(),
        panel.background = element_blank(),
        panel.border = element_rect(colour = "black", fill=NA,linewidth = 2)) + 
  xlab("Filled Cell Density") + ylab("Avg. Variables") + 
  theme(text = element_text(size = 30,family="Times New Roman")) + 
  scale_y_continuous(labels = label_comma())
ggsave(filename = '/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Spring/Thesis Visuals/Chapter 5/Pretty/variableCount.tiff',
       device='tiff', dpi=1200)
