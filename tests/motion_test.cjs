const assert=require('node:assert/strict');
const MobileWander=require('../AquariumHome/Motion.js');
function rng(seed){return()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296}}
const all=['large','small','octopus','crab'].map((id,i)=>({id,owner:'phone',entryY:[145/240,70/240,204/240,220/240][i]}));
let directionChanges=0,stoppedAfterTwo=0;
for(let seed=1;seed<=20;seed++){
 const m=new MobileWander(rng(seed)),dirs=Object.fromEntries(all.map(f=>[f.id,new Set()]));
 for(const [i,f]of all.entries())m.positions[f.id]={x:[12,116,95,178][i],y:f.entryY,dir:i%2?-1:1,entering:false,vx:i%2?-25:25,vy:0,next:0,crossed:false};
 for(let frame=0;frame<12000;frame++){
  const events=m.step(all,.05,frame*.05,240,240,false);assert.equal(events.length,0);
  for(const f of all){const p=m.positions[f.id];assert(!p.crossed);dirs[f.id].add(p.dir);}
 }
 for(const f of all){assert.equal(dirs[f.id].size,2,`not random in both directions: ${f.id}, seed=${seed}`);directionChanges++;}
 let residents=all.slice(),crossed=new Set();
 for(let frame=0;frame<16000&&crossed.size<2;frame++){
  for(const id of m.step(residents,.05,600+frame*.05,240,240,true))crossed.add(id);
  residents=all.filter(f=>!crossed.has(f.id));
 }
 assert.equal(crossed.size,2,`two never reach a natural edge, seed=${seed}`);
 for(let frame=0;frame<3000;frame++)assert.equal(m.step(residents,.05,1500+frame*.05,240,240,false).length,0);
 assert.equal(crossed.size,2);stoppedAfterTwo++;
}
console.log(`PASS ${directionChanges} independently bidirectional creature runs; ${stoppedAfterTwo} take-two-then-close scenarios; no crossings while closed`);
