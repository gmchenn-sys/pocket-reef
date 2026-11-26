const assert=require('node:assert/strict'),Motion=require('../AquariumHome/Motion');
const all=['large','small','octopus','crab'].map(id=>({id}));let separationChecks=0,bites=0;
for(let seed=1;seed<=40;seed++){
 let n=seed;const m=new Motion(()=>{n=(Math.imul(n,1664525)+1013904223)>>>0;return n/4294967296});
 all.forEach((f,i)=>m.positions[f.id]={x:[110,20,30,160][i],y:[.6,.25,.75,220/240][i],dir:i%2?-1:1,vx:17,vy:0,next:20,entering:false,crossed:false});
 const x=m.positions.large.x,dir=m.positions.large.dir;m.turn('large',0);assert.equal(m.positions.large.dir,-dir);assert.equal(m.positions.large.x,x);
 m.feed(130,70,all,0,240,240);assert.equal(m.food.length,12);const startX=m.food.map(q=>q.x);let highCrab=220,previous=0;const seen=new Set(),last={};
 for(let frame=0;frame<900;frame++){
  const t=frame*.05;m.step(all,.05,t,240,240,false);highCrab=Math.min(highCrab,m.positions.crab.y*240);
  for(const event of m.consumed.slice(previous)){assert(!seen.has(event.id),'one particle eaten twice');seen.add(event.id);assert(!m.food.some(q=>q.id===event.id),'eaten grain still drawn');if(last[event.by]!==undefined)assert(event.t-last[event.by]>=.64,'rapid repeated chewing');last[event.by]=event.t;bites++;}
  previous=m.consumed.length;assert.equal(m.food.length+m.consumed.length,12,'food conservation before expiry');
  for(let i=0;i<4;i++)for(let j=i+1;j<4;j++){const a=m.bounds(all[i],m.positions[all[i].id],240),b=m.bounds(all[j],m.positions[all[j].id],240);if(a.right<0||a.left>=240||b.right<0||b.left>=240)continue;assert(!(a.left<b.right+5.99&&a.right+5.99>b.left&&a.top<b.bottom+5.99&&a.bottom+5.99>b.top),`feeding overlap seed${seed}`);separationChecks++;}
  if(frame===20)assert(m.food.some(q=>Math.abs(q.x-startX[q.id-1])>1),'food never drifts');
 }
 for(const f of all)assert(m.eaten[f.id]>0,`no food for ${f.id}, seed${seed}`);assert(highCrab<200,'crab never approaches upper food');
 for(let i=900;i<1200;i++)m.step(all,.05,i*.05,240,240,false);
 assert.equal(m.feeding,null);assert(m.food.length===0);assert(Object.values(m.positions).every(p=>!p.chewing));
}
console.log(`PASS ${bites} unique one-grain bites, ${separationChecks} feeding separation checks, all four eat across 40 seeds, crab ascends, grains drift and consumed IDs never reappear`);
