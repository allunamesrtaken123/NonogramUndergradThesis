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

puzzlePageLinks = ['https://www.nonograms.org/nonograms'] + [f'https://www.nonograms.org/nonograms/p/{i}' for i in range(2,1141)]

failedLinks = []
with open("/Users/aaronfoote/COURSES/Krizanc-Tutorials/Senior-Winter/Scraping Puzzles/puzzleLinksNewer.txt","w") as f:
    #f.write("index,tilesToInfer,rows,columns,difficulty\n")

    driver = webdriver.Chrome(options = options) 

    for pl in tqdm(puzzlePageLinks):
        try:
            driver.get(pl)
            linkElements = driver.find_elements(By.CLASS_NAME,"nonogram_title")
            for l in [el.get_attribute('href') for el in linkElements]:
                f.write(l)
                f.write("\n")
        except:
            failedLinks.append(pl)
    while failedLinks:
        for pl in failedLinks:
            try:
                driver.get(pl)
                linkElements = driver.find_elements(By.CLASS_NAME,"nonogram_title")
                for l in [el.get_attribute('href') for el in linkElements]:
                    f.write(l)
                    f.write("\n")
                    failedLinks.remove(pl)
            except:
                pass
    driver.close()
    driver.quit()
f.close()