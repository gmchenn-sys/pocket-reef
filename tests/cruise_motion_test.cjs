const assert=require('node:assert/strict'),Mobile=require('../AquariumHome/Motion');
const creatures=['large','small','octopus','crab'].map(id=>({id}));let checks=0,turns=0;
for(let seed=1;seed<=30;seed++){
 let n=seed;const rng=()=>{n=(Math.imul(n,1664525)+1013904223)>>>0;return n/4294967296},m=new Mobile(rng);
 creatures.forEach((f,i)=>m.positions[f.id]={x:[12,116,95,178][i],y:[145/240,70/240,204/240,220/240][i],dir:i%2?-1:1,vx:17,vy:0,next:0,entering:false});
 const lastTurn={},lastDir={},lastX={};
 for(let frame=0;frame<10000;frame++){
  const t=frame*.05;m.step(creatures,.05,t,240,240,false);
  for(const f of creatures){const p=m.positions[f.id];
   if(lastDir[f.id]&&lastDir[f.id]!==p.dir){if(lastTurn[f.id]!==undefined)assert(t-lastTurn[f.id]>= (f.id==='crab'?3.49:4.49),`rapid turn ${f.id}`);lastTurn[f.id]=t;turns++;}
   // Wrapping must not reverse the heading.
   if(f.id!=='crab'&&lastX[f.id]!==undefined&&Math.abs(p.x-lastX[f.id])>100)assert.equal(p.dir,lastDir[f.id]);lastDir[f.id]=p.dir;lastX[f.id]=p.x;
  }
  for(let i=0;i<4;i++)for(let j=i+1;j<4;j++){
   const a=m.bounds(creatures[i],m.positions[creatures[i].id],240),b=m.bounds(creatures[j],m.positions[creatures[j].id],240);
   if(a.right<0||a.left>=240||b.right<0||b.left>=240)continue;
   assert(!(a.left<b.right+5.99&&a.right+5.99>b.left&&a.top<b.bottom+5.99&&a.bottom+5.99>b.top),`body overlap seed=${seed} frame=${frame}`);checks++;
  }
 }
}
console.log(`PASS ${checks} body-separation checks, ${turns} spaced turns, no heading reversal at wrapping edges`);
