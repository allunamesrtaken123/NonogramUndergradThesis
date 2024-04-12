
from tqdm import tqdm
from time import strftime
from selenium import webdriver 
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
import os
import json

options = Options()
options.add_argument('--headless=new')
#options.add_argument("--headless=new")
driver = webdriver.Chrome(options = options) 

"""
with open('/Users/aaronfoote/COURSES/Krizanc Tutorials/Senior Winter/Scraping Puzzles/scrapedNonogram.csv','w') as f:
    f.write("index,tilesInferred,rows,columns,difficulty\n")
    for i in tqdm(range(1,10)):
        

            f.write(f'{i},{tilesToInfer},{rows},{columns},{difficulty}\n')
f.close()


driver.get(f'https://www.nonograms.org/nonograms/i/{7}') 
if not driver.find_elements(By.CLASS_NAME,'message'): # You get a message with it's a bad link
    runs = driver.find_elements(By.CLASS_NAME,"num")
    tilesToInfer = sum([int(el.text) for el in runs])/2

    columnDescriptions = driver.find_elements(By.CLASS_NAME,"nmtt")
    columnDescriptionRows = columnDescriptions[0].find_elements(By.TAG_NAME,"tr")[0]
    columns = len(columnDescriptionRows.find_elements(By.TAG_NAME,"td"))

    rowDescriptions = driver.find_elements(By.CLASS_NAME,"nmtl")[0]
    rows = len( rowDescriptions.find_elements(By.TAG_NAME,"tr"))

    content = driver.find_element(By.CLASS_NAME,"content") # Only one content
    table = content.find_element(By.TAG_NAME,"table") # Multiple tables, but always the first
    tr = table.find_elements(By.TAG_NAME,"tr")
    #text = driver.find_elements(By.TAG_NAME,"img")[0].get_attribute("title")
    #print(text)
    #difficulty = text.get_attribute("title").split("/")[0]


This gets the stats for links that are actually puzzles
runs = driver.find_elements(By.CLASS_NAME,"num") # Gives row AND column runs
#print(leagues)
totalTiles = sum([int(el.text) for el in runs])/2 # Divide by two, as the sum double counts each cell (in row AND column)
#links = [el.text for el in leagues]
print(f"Tiles to Be Inferred: {totalTiles}")

columnDescriptions = driver.find_elements(By.CLASS_NAME,"nmtt")
columnDescriptionRows = columnDescriptions[0].find_elements(By.TAG_NAME,"tr")[0]
columns = len(columnDescriptionRows.find_elements(By.TAG_NAME,"td"))
print(f"Columns: {columns}")

rowDescriptions = driver.find_elements(By.CLASS_NAME,"nmtl")[0]
rows = len( rowDescriptions.find_elements(By.TAG_NAME,"tr"))
#rows = len(rowDescriptionCells.find_elements(By.TAG_NAME,"td"))
print(f"Rows: {rows}")

content = driver.find_element(By.CLASS_NAME,"content")
table = content.find_element(By.TAG_NAME,"table")
tr = table.find_element(By.TAG_NAME,"tr")
text = tr.find_elements(By.TAG_NAME,"img")[1]
difficulty = text.get_attribute("title").split("/")[0]
print(difficulty)
"""



"""
For each puzzle I want:
- The ID of the puzzle (number that's tacked on the end)
- The total number of tiles to be inferred 
- The size of the board (height * width)
- Difficulty rating

Right now I can get:
- Index of the puzzle
- Total number of tiles to be inferred
- Size of the board
- Difficulty rating
Still need to get:


Also need to functionize the thing so that it takes a link and just outputs a string that I can immediately
write to file. I should definite parallelize it to at least twenty puzzles at a time if possible, because
otherwise it will take forever. Given the pretty format of the puzzle links, I can also don't have to do 
it in one sitting and can process them over time
"""

f1 = open("/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/puzzleLinksNewer.txt", "r")
puzzleLinks = f1.readlines()

failedLinks = []

driver = webdriver.Chrome(options = options) 

for pl in tqdm(puzzleLinks):
    try:
        driver.get(pl)
        columnDescriptions = driver.find_element(By.CLASS_NAME,"nmtt")
        columnDescriptionRows = columnDescriptions.find_elements(By.TAG_NAME,"tr")

        columnJSON = [[] for _ in range(len(columnDescriptionRows[0].find_elements(By.TAG_NAME,"td")))]

        for tr in columnDescriptionRows:
            for i,td in enumerate(tr.find_elements(By.TAG_NAME,"td")):
                if td.get_attribute("class") in ['num','num nmt_td5']:
                    div = td.find_element(By.TAG_NAME,"div")
                    columnJSON[i].append(int(div.text))
        """
        print("Column Descriptions:")
        for c in columnJSON:
            print(c)
        """
        rowDescriptions = driver.find_element(By.CLASS_NAME,"nmtl").find_elements(By.TAG_NAME,"tr")
        rowJSON = []
        for tr in rowDescriptions:
            tds = tr.find_elements(By.TAG_NAME,"td")
            description = []

            for td in tds:
                if td.get_attribute("class") == "num":
                    description.append(int(td.find_element(By.TAG_NAME,"div").text))
            
            rowJSON.append(description)
        """
        print("Row Descriptions:")
        for r in rowJSON:
            print(r)
        """
        i = pl.split("/")[-1].strip()
        boardJSON = {
            'rows' : rowJSON,
            'columns' : columnJSON,
            'rowCount' : len(rowJSON),
            'columnCount' : len(columnJSON)}
        with open(f"/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/puzzleJSONdir/{i}.json",'w') as f:
            json.dump(boardJSON,f)
    except:
        failedLinks.append(pl)
while failedLinks:
    for pl in failedLinks:
        try:
            driver.get(pl)
            columnDescriptions = driver.find_element(By.CLASS_NAME,"nmtt")
            columnDescriptionRows = columnDescriptions.find_elements(By.TAG_NAME,"tr")

            columnJSON = [[] for _ in range(len(columnDescriptionRows[0].find_elements(By.TAG_NAME,"td")))]

            for tr in columnDescriptionRows:
                for i,td in enumerate(tr.find_elements(By.TAG_NAME,"td")):
                    if td.get_attribute("class") in ['num','num nmt_td5']:
                        div = td.find_element(By.TAG_NAME,"div")
                        columnJSON[i].append(int(div.text))
            """
            print("Column Descriptions:")
            for c in columnJSON:
                print(c)
            """
            rowDescriptions = driver.find_element(By.CLASS_NAME,"nmtl").find_elements(By.TAG_NAME,"tr")
            rowJSON = []
            for tr in rowDescriptions:
                tds = tr.find_elements(By.TAG_NAME,"td")
                description = []

                for td in tds:
                    if td.get_attribute("class") == "num":
                        description.append(int(td.find_element(By.TAG_NAME,"div").text))
                
                rowJSON.append(description)
            """
            print("Row Descriptions:")
            for r in rowJSON:
                print(r)
            """
            i = pl.split("/")[-1].strip()
            boardJSON = {
                'rows' : rowJSON,
                'columns' : columnJSON,
                'rowCount' : len(rowJSON),
                'columnCount' : len(columnJSON)}
            with open(f"/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/puzzleJSONdir/{i}.json",'w') as f:
                json.dump(boardJSON,f)
                failedLinks.remove(pl)
        except:
            pass
driver.close()
driver.quit()
f.close()



"""driver.get(f'https://www.nonograms.org/nonograms/i/{7}') 

columnDescriptions = driver.find_element(By.CLASS_NAME,"nmtt")
columnDescriptionRows = columnDescriptions.find_elements(By.TAG_NAME,"tr")

columnJSON = [[] for _ in range(len(columnDescriptionRows[0].find_elements(By.TAG_NAME,"td")))]

for tr in columnDescriptionRows:
    for i,td in enumerate(tr.find_elements(By.TAG_NAME,"td")):
        if td.get_attribute("class") in ['num','num nmt_td5']:
            div = td.find_element(By.TAG_NAME,"div")
            columnJSON[i].append(int(div.text))
print("Column Descriptions:")
for c in columnJSON:
    print(c)

rowDescriptions = driver.find_element(By.CLASS_NAME,"nmtl").find_elements(By.TAG_NAME,"tr")
rowJSON = []
for tr in rowDescriptions:
    tds = tr.find_elements(By.TAG_NAME,"td")
    description = []

    for td in tds:
        if td.get_attribute("class") == "num":
            description.append(int(td.find_element(By.TAG_NAME,"div").text))
    
    rowJSON.append(description)

print("Row Descriptions:")
for r in rowJSON:
    print(r)


boardJSON = {
    'rows' : rowJSON,
    'columns' : columnJSON,
    'rowCount' : len(rowJSON),
    'columnCount' : len(columnJSON)}
with open(f"./puzzleJSONdir/7.json",'w') as f:
    json.dump(boardJSON,f)
"""
"""
runs = driver.find_elements(By.CLASS_NAME,"num")
tilesToInfer = sum([int(el.text) for el in runs])/2

columnDescriptions = driver.find_elements(By.CLASS_NAME,"nmtt")
columnDescriptionRows = columnDescriptions[0].find_elements(By.TAG_NAME,"tr")[0]
columns = len(columnDescriptionRows.find_elements(By.TAG_NAME,"td"))

rowDescriptions = driver.find_elements(By.CLASS_NAME,"nmtl")[0]
rows = len( rowDescriptions.find_elements(By.TAG_NAME,"tr"))

content = driver.find_element(By.CLASS_NAME,"content") # Only one content
table = content.find_element(By.TAG_NAME,"table") # Multiple tables, but always the first
tr = table.find_elements(By.TAG_NAME,"tr")
"""