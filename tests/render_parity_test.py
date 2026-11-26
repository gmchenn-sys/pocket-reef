from pathlib import Path
import subprocess,json
root=Path(__file__).resolve().parents[1];src=(root/'AquariumHome/AquariumHome.ino').read_text()
octo=src[src.index('void drawOctopus('):src.index('void creatureBubbles(')]
crab=src[src.index('void drawCrab('):src.index('void loop()')]
fish=src[src.index('  for (auto &f : fish) {',src.index('void loop()')):src.index('    float mouthX =',src.index('void loop()'))]+'  }\n'
header=r'''#include <cstdio>
#include <cmath>
#include <cstdint>
constexpr int W=240;
uint16_t rgb(int r,int g,int b){return ((r&248)<<8)|((g&252)<<3)|(b>>3);}
const uint16_t EYE_INK=rgb(226,217,139),FIN_INK=rgb(195,222,214),ST77XX_WHITE=65535;
struct Canvas {int x,y,size;uint16_t color;void setTextSize(int s){size=s;}void setTextColor(uint16_t c){color=c;}void setCursor(int a,int b){x=a;y=b;}void write(char c){printf("G %d %d %d %d %u\n",int(c),x,y,size,color);x+=6*size;}void print(const char*s){while(*s)write(*s++);}void drawLine(int x,int y,int a,int b,uint16_t c){printf("L %d %d %d %d %u\n",x,y,a,b,c);}void drawPixel(int x,int y,uint16_t c){printf("P %d %d %u\n",x,y,c);}}canvas;
struct Fish {float x,y,speed,phase;int direction;uint8_t scale;uint16_t color;}fish[2]={{0,0,17,.1f,1,2,rgb(169,220,215)},{0,0,15,4.8f,-1,1,rgb(208,197,232)}};
struct Wander{float x,y;int direction;}swimmers[3];float crabX,crabY=220;int selected;
bool fishInTank(int i){return i==selected;}
bool fishVisible(int i){return i==selected;}
'''
plants=src[src.index('void drawPlants('):src.index('void drawOctopus(')]
main='''int main(){float ts[]={0,1.25,6.5,20};for(float t:ts)for(int dir:{-1,1})for(selected=0;selected<4;selected++){printf("CASE %.2f %d %d\\n",t,dir,selected);for(auto&m:swimmers){m.x=63.25;m.y=122.5;m.direction=dir;}crabX=63.25;if(selected<2)drawFish(t);else if(selected==2){swimmers[2].x+=18;drawOctopus(t);}else drawCrab(t);}for(float t:ts){printf("PLANTS %.2f\\n",t);drawPlants(t);}}'''
cpp=Path('/tmp/reef-render-parity.cpp');cpp.write_text(header+'\n'+octo+crab+plants+'\nvoid drawFish(float t){\n'+fish+'}\n'+main.replace('#include',''))
# initializer-list iteration needs the standard declaration.
cpp.write_text('#include <initializer_list>\n'+cpp.read_text())
subprocess.run(['c++','-std=c++17',str(cpp),'-o','/tmp/reef-render-parity'],check=True)
expected=subprocess.check_output(['/tmp/reef-render-parity'],text=True)
js=r'''
const fs=require('fs'),s=fs.readFileSync('AquariumHome/app.js','utf8');
const fun=s.slice(s.indexOf('function drawCreature('),s.indexOf('function draw(ms)'));
const commands=[],ink=c=>{const n=parseInt(c.slice(1),16);return ((n>>16&248)<<8)|((n>>8&252)<<3)|((n&255)>>3)};
const glyph=(c,x,y,z,color)=>commands.push(`G ${c.charCodeAt(0)} ${Math.trunc(x)} ${Math.trunc(y)} ${z} ${ink(color)}`);
const textPixels=(s,x,y,z,c)=>[...s].forEach((v,i)=>glyph(v,x+i*6*z,y,z,c));
const line=(x,y,a,b,c)=>commands.push(`L ${Math.trunc(x)} ${Math.trunc(y)} ${Math.trunc(a)} ${Math.trunc(b)} ${ink(c)}`);
const pixel=(x,y,c)=>commands.push(`P ${Math.trunc(x)} ${Math.trunc(y)} ${ink(c)}`);
const plants=s.slice(s.indexOf('function drawPlants('),s.indexOf('function draw(ms)'));
const drawPlants=new Function('line','pixel',plants+';return drawPlants')(line,pixel);
const ctx={beginPath(){},arc(){},stroke(){}};
const draw=new Function('glyph','textPixels','line','span','height','width','ctx','pixel','motion',fun+';return drawCreature')(glyph,textPixels,line,f=>f.id==='large'?84:f.id==='small'?42:36,240,240,ctx,pixel,{feeding:null});
for(const t of [0,1.25,6.5,20])for(const dir of [-1,1])for(const [i,id]of ['large','small','octopus','crab'].entries()){commands.push(`CASE ${t.toFixed(2)} ${dir} ${i}`);draw({id,color:i===0?'#a9dcd7':'#d0c5e8'},{x:63.25,y:(id==='crab'?220:122.5)/240,dir},t)}for(const t of [0,1.25,6.5,20]){commands.push(`PLANTS ${t.toFixed(2)}`);drawPlants(t)}console.log(commands.join('\n'));
'''
actual=subprocess.check_output(['node','-e',js],cwd=root,text=True)
a=expected.splitlines();b=actual.splitlines()
assert len(a)==len(b),(len(a),len(b))
for i,(x,y)in enumerate(zip(a,b)):
 assert x==y,(i,x,y)
print('PASS 32 native/browser render fixtures: identical glyphs, sizes, coordinates, RGB565 colors, fins, crab legs and octopus arms; 4 plant/sand frames match')
