const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const Near=require('../AquariumHome/Proximity'),{ReefLink}=Near;
const link=new ReefLink(),near=new Near();let now=1000;
for(let i=0;i<30;i++){near.update(-30,now);now+=250;}
assert(near.near);link.received(now);
// Two lost/slow responses are tolerated; no indefinitely latched connection.
link.failed();link.failed();assert(link.live(now+4500));assert(near.fresh(now+4500));
assert.equal(link.status(now+4500),'retrying');assert(!link.live(now+6000));
assert(!near.fresh(now+6000));link.received(now+6100);assert.equal(link.status(now+6100),'connected');
for(let i=0;i<30;i++)near.update(-78,now+6200+i*250);
assert(!near.near,'real retreat must close, independent of transport grace');

// Exercise the actual app callbacks, including fetch errors and near=0/1 POSTs.
const nodes=new Map();const ctx=new Proxy({}, {get:(o,k)=>o[k]||(()=>{})});
function node(id){if(!nodes.has(id))nodes.set(id,{textContent:id==='feedback'?'正在连接鱼缸…':'',style:{},className:'',clientWidth:240,clientHeight:240,getContext:()=>ctx,replaceChildren(){},append(){},addEventListener(){}});return nodes.get(id);}
const document={getElementById:node,hidden:false,createElement:()=>node(Math.random()),addEventListener(){}};
let clock=100000,mode='ok',seq=0,bodies=[],timers=[];const storage=new Map();
const sandbox={console,URLSearchParams,AbortController,Date:{now:()=>clock},Math,performance:{now:()=>clock},document,localStorage:{getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},matchMedia:()=>({matches:false}),devicePixelRatio:1,ResizeObserver:class{observe(){}},requestAnimationFrame(){},setInterval(){},addEventListener(){},setTimeout:(f,ms)=>{timers.push(ms);return timers.length},clearTimeout(){},window:{},fetch:async(path,options)=>{bodies.push(String(options.body));if(mode==='fail')throw new Error('temporary loss');return {ok:true,json:async()=>({boot:1,seq:++seq,serverMs:clock,revision:0,rssi:mode==='null'?null:-30,portalOpen:String(options.body).includes('near=1'),portalEpoch:2,creatures:[],seam:null})}}};
vm.createContext(sandbox);
for(const f of ['Proximity.js','Seam.js','Motion.js'])vm.runInContext(fs.readFileSync('AquariumHome/'+f,'utf8'),sandbox);
let source=fs.readFileSync('AquariumHome/app.js','utf8').replace('restoreResidents();render();refresh();','window.test={refresh,canCross,near,link,closePortal};restoreResidents();render();');
vm.runInContext(source,sandbox);const api=sandbox.window.test;
(async()=>{
 for(let i=0;i<12;i++){await api.refresh();clock+=300;}
 assert(api.canCross());const before=api.link.lastGood;
 mode='fail';clock=before+2100;await api.refresh();assert(api.canCross(),'one slow poll must not close');
 assert.equal(node('connection').textContent,'连接重试中');
 clock=before+4500;await api.refresh();assert(api.canCross(),'second lost poll within lease must not close');
 mode='ok';clock=before+4800;await api.refresh();assert(bodies.at(-1).includes('near=1'),'retry must renew, not explicitly close');
 assert(api.canCross());assert.equal(node('feedback').textContent,'');
 mode='null';clock+=2500;await api.refresh();assert(api.canCross(),'transient missing RSSI must not close');
 mode='ok';clock+=200;await api.refresh();assert(api.canCross());
 mode='fail';clock+=6100;await api.refresh();assert(!api.canCross());
 assert.equal(node('connection').textContent,'离线陪伴');
 mode='ok';await api.refresh();await api.refresh();assert(api.canCross(),'reconnect recovers after authoritative ack');
 document.hidden=true;assert(!api.canCross());document.hidden=false;
 api.closePortal();assert(!api.canCross(),'explicit close is immediate');
 assert(timers.every(ms=>ms===1800));
 console.log('PASS actual app: slow/lost polls, renew after delay, missing RSSI, eventual outage, reconnect, foreground close; no forced ownership changes');
})().catch(e=>{console.error(e);process.exitCode=1});
