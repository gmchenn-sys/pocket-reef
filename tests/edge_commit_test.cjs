const assert=require('node:assert/strict'),Motion=require('../AquariumHome/Motion');
for(const id of ['large','small','octopus','crab'])for(const feed of [false,true]){
 const f={id},m=new Motion(()=>0);m.positions[id]={x:0,y:id==='crab'?220/240:.5,dir:-1,vx:20,vy:0,next:.05,entering:false,crossed:false};
 if(feed)m.feed(180,110,[f],0,240,240);
 let crossed=false;
 for(let frame=0;frame<160;frame++){
  const events=m.step([f],.05,frame*.05,240,240,true),p=m.positions[id];
  assert.equal(p.dir,-1,`${id} reversed during exit`);
  if(events.includes(id)){assert(m.bounds(f,p,240).right<=0);crossed=true;break;}
 }
 assert(crossed,`${id} failed to finish edge exit, feeding=${feed}`);
}
for(const id of ['large','small','octopus','crab']){
 const f={id},m=new Motion(()=>.5);m.positions[id]={x:0,y:id==='crab'?220/240:.5,dir:-1,vx:20,vy:0,next:10,entering:false,crossed:false};
 m.step([f],.05,0,240,240,true);assert(m.positions[id].edgeExit);
 for(let frame=1;frame<160;frame++)assert.equal(m.step([f],.05,frame*.05,240,240,false).length,0,`${id} crossed closed portal`);
 assert(!m.positions[id].edgeExit);
 const incoming=new Motion(()=>.5);incoming.enter(f,240,240);assert.equal(incoming.step([f],.05,0,240,240,true).length,0);assert.equal(incoming.positions[id].dir,1);
}
console.log('PASS all four finish shared-edge departures despite random-turn deadlines/food; closing cancels; arriving creatures excluded');
