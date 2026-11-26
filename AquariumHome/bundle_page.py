from pathlib import Path
root=Path(__file__).resolve().parent
page=(root/'index.html').read_text()
a=page.index('<script>');b=page.index('</script>',a)
page=page[:a]+'<script>\n'+'\n'.join((root/n).read_text() for n in ['Glyphs.js','Seam.js','Proximity.js','Motion.js','app.js'])+page[b:]
(root/'index.html').write_text(page)
(root/'WebPage.h').write_text('#pragma once\nconst char WEB_PAGE[] PROGMEM = R"REEFPAGE('+page+')REEFPAGE";\n')
