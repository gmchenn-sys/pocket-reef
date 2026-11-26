const assert=require('node:assert/strict'),Motion=require('../AquariumHome/Motion'),View=require('../AquariumHome/Seam');
for(const id of ['large','small','octopus','crab']){
 const m=new Motion(()=>0),f={id};m.seamEnabled=true;m.positions[id]={x:id==='crab'?2:1,y:id==='crab'?220/240:.5,dir:-1,vx:20,vy:0,next:.01,entering:false};let event=false;
 for(let i=0;i<4;i++)event= m.step([f],.05,i*.05,240,240,true).includes(id)||event;
 assert(event&&m.positions[id].seamWaiting);assert(m.bounds(f,m.positions[id],240).right>0,'source disappears before start acknowledgement');
 const w=m.span(f),v=new View();v.apply({fish:id,phase:'running',toPhone:true,width:w,started:1000,duration:3000,height:.5},1000,1);
 for(const u of [.02,.1,.25,.5,.8,.98]){let t=1+u*3;const a=v.pose(t,false),b=v.pose(t,true);const al=a.x-(id==='octopus'?18:id==='crab'?1:0),bl=b.x-(id==='crab'?1:0);const visible=x=>Math.max(0,Math.min(240,x+w)-Math.max(0,x));assert(visible(al)>0&&visible(bl)>0);assert(Math.abs(visible(al)+visible(bl)-w)<1e-6);}
 m.step([f],.05,1,240,240,false);assert(!m.positions[id].seamWaiting,'closed edge left source stuck');
}
console.log('PASS front-contact freeze, early simultaneous slices, width conservation and closed-edge release for all four creatures');

const order=new View.SnapshotOrder();assert(order.accept({boot:1,seq:10}).accepted);assert(!order.accept({boot:1,seq:9}).accepted);assert(order.accept({boot:2,seq:1}).reset);assert(!order.accept({boot:1,seq:99}).accepted);assert(order.accept({boot:2,seq:2}).accepted);console.log('PASS out-of-order frames ignored, fresh reboot accepted, retired boot cannot rewind ownership or crossing');

const reserved=new Motion(),resident={id:'small'};reserved.seamReservation={id:'large',left:0,right:84,top:110,bottom:157};assert(!reserved.room(resident,{x:100,y:.5},70,.5,[],240,240));assert(reserved.room(resident,{x:150,y:.2},150,.2,[],240,240));console.log('PASS arriving body corridor reserved without trapping an occupied offer');

const moving=new View();moving.apply({fish:'large',phase:'offer',toPhone:false,width:84,lead:48,started:1000,duration:132/17*1000,height:.5},1000,1);const startX=moving.pose(1,true).x;assert(Math.abs(startX-48)<1e-6);const oldX=moving.pose(1.6,true).x;moving.state.phase='running';assert.equal(moving.pose(1.6,true).x,oldX);for(let t=3.4;t<4.2;t+=.01)assert(Math.abs((moving.pose(t+.01,true).x-moving.pose(t,true).x)+.17)<.0001);console.log('PASS browser preflight moves continuously through acknowledgement and edge');
