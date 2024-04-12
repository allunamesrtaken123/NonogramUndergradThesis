from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from tqdm import tqdm

options = Options()
options.add_argument("enable-automation");
options.headless = True
options.add_argument("--headless=new");
options.add_argument("--window-size=1920,1080");
options.add_argument("--no-sandbox");
options.add_argument("--disable-extensions");
options.add_argument("--dns-prefetch-disable");
options.add_argument("--disable-gpu");

f1 = open("/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/newLinks.txt", "r")
puzzleLinks = f1.readlines()

failedLinks = []
with open("/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/scrapedNonogram.csv","a") as f:
    #f.write("index,tilesToInfer,rows,columns,difficulty\n")

    driver = webdriver.Chrome(options = options) 

    for pl in tqdm(puzzleLinks):
        try:
            driver.get(pl)

            runs = driver.find_elements(By.CLASS_NAME,"num")
            tilesToInfer = sum([int(el.text) for el in runs])/2

            columnDescriptions = driver.find_element(By.CLASS_NAME,"nmtt")
            columnDescriptionRows = columnDescriptions.find_element(By.TAG_NAME,"tr")
            columns = len(columnDescriptionRows.find_elements(By.TAG_NAME,"td"))

            rowDescriptions = driver.find_element(By.CLASS_NAME,"nmtl")
            rows = len(rowDescriptions.find_elements(By.TAG_NAME,"tr"))

            content = driver.find_element(By.CLASS_NAME,"content")
            table = content.find_element(By.TAG_NAME,"table")
            tr = table.find_element(By.TAG_NAME,"tr")
            images = tr.find_elements(By.TAG_NAME,"img")[1]
            difficulty = images.get_attribute("title").split("/")[0]

            i = pl.split("/")[-1].strip()

            f.write(f'{i},{tilesToInfer},{rows},{columns},{difficulty}\n')
        except:
            failedLinks.append(pl)
    while failedLinks:
        for pl in failedLinks:
            try:
                driver.get(pl)

                runs = driver.find_elements(By.CLASS_NAME,"num")
                tilesToInfer = sum([int(el.text) for el in runs])/2

                columnDescriptions = driver.find_element(By.CLASS_NAME,"nmtt")
                columnDescriptionRows = columnDescriptions.find_element(By.TAG_NAME,"tr")
                columns = len(columnDescriptionRows.find_elements(By.TAG_NAME,"td"))

                rowDescriptions = driver.find_element(By.CLASS_NAME,"nmtl")
                rows = len(rowDescriptions.find_elements(By.TAG_NAME,"tr"))

                content = driver.find_element(By.CLASS_NAME,"content")
                table = content.find_element(By.TAG_NAME,"table")
                tr = table.find_element(By.TAG_NAME,"tr")
                images = tr.find_elements(By.TAG_NAME,"img")[1]
                difficulty = images.get_attribute("title").split("/")[0]

                i = pl.split("/")[-1].strip()

                f.write(f'{i},{tilesToInfer},{rows},{columns},{difficulty}\n')
                failedLinks.remove(pl)
            except:
                pass
    driver.close()
    driver.quit()
f.close()
