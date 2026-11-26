const assert=require('node:assert/strict'),Near=require('../AquariumHome/Proximity');let t=1000;
function samples(p,v,n){for(let i=0;i<n;i++){t+=250;p.update(typeof v==='function'?v(i):v,t)}}
const quiet=new Near();samples(quiet,i=>-60+Math.sin(i)*2,160);assert.equal(quiet.near,false);
const near=new Near();samples(near,-65,20);samples(near,-30,24);assert(near.near);samples(near,-70,24);assert(!near.near);samples(near,-30,24);assert(near.near);
const brief=new Near();samples(brief,-60,20);samples(brief,-25,2);samples(brief,-60,12);assert(!brief.near);
const bad=new Near();samples(bad,null,20);assert(!bad.near);
const touching=new Near();samples(touching,0,24);assert(touching.near);samples(touching,i=>[0,-15,-35,-44,-30][i%5],160);assert(touching.near,'close signal fluctuations must not shut gate');
samples(touching,-90,1);assert(touching.near,'one weak outlier must not close');samples(touching,-35,8);assert(touching.near);samples(touching,-75,20);assert(!touching.near,'sustained retreat closes');
const actual=new Near();samples(actual,-44,24);assert(actual.near,'actual touching RSSI must open');samples(actual,-44,240);assert(actual.near);samples(actual,-68,24);assert(!actual.near);
const calibrated=new Near();calibrated.threshold=-2;samples(calibrated,-30,24);assert(calibrated.near,'saturated calibration must not make exit brittle');samples(calibrated,-42,40);assert(calibrated.near);samples(calibrated,-75,24);assert(!calibrated.near);
console.log('PASS actual -44dBm proximity, saturated zero calibration, close-range fluctuations, weak outlier, sustained retreat/reapproach, invalid readings');
