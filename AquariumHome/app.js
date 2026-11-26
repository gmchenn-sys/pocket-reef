(()=>{
const $=id=>document.getElementById(id),store={get(k){try{return localStorage.getItem(k)}catch{return null}},set(k,v){try{localStorage.setItem(k,v)}catch{}},remove(k){try{localStorage.removeItem(k)}catch{}}};
let client=store.get('reef-client');if(!client){client='p-'+Math.random().toString(36).slice(2)+Date.now().toString(36);store.set('reef-client',client)}
let state=null;try{state=JSON.parse(store.get('reef-state')||'null')}catch{}if(state&&!state.creatures)state=null;
let online=false,refreshing=false,busy=false,autoEnabled=store.get('reef-auto')!=='off',portalAck=false,portalEpoch=0,portalAckAt=0,lastCrossState=false,authoritative=false;
const near=new NearApproach(),link=new ReefLink(),motion=new MobileWander(),seamView=new SeamView(),snapshots=new SnapshotOrder(),positions=motion.positions,names={large:'大鱼',small:'小鱼',octopus:'小乌贼',crab:'小螃蟹'};
const canvas=$('ocean'),ctx=canvas.getContext('2d');let width=240,height=240,previous=0;
const savedThreshold=store.get('reef-near-threshold');if(savedThreshold!==null&&Number.isFinite(Number(savedThreshold)))near.threshold=Number(savedThreshold);
motion.seamEnabled=true;let seamSending=false,seamRetry=0,lastPoll=0;
const reduced=matchMedia('(prefers-reduced-motion: reduce)').matches,spriteScale=1;
const animals=()=>state?.creatures||[],span=f=>motion.span(f);
function pending(){try{return JSON.parse(store.get('reef-seam-pending')||'null')}catch{return null}}
function canCross(){return link.live(Date.now())&&autoEnabled&&near.near&&near.fresh(Date.now())&&portalAck&&Date.now()-portalAckAt<ReefLink.FRESH_MS&&!document.hidden}
function say(s){$('feedback').textContent=s}
async function api(path,options={}){const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),ReefLink.REQUEST_MS);try{const clockStart=performance.now()/1000;const response=await fetch(path,{...options,cache:'no-store',signal:controller.signal});const result=await response.json();result._clockLocal=(clockStart+performance.now()/1000)/2;if(!response.ok)throw Object.assign(new Error(result.error||'未确认操作'),{status:response.status,state:result});return result}finally{clearTimeout(timer)}}
function apply(next,sample=true){
 const verdict=snapshots.accept(next);if(!verdict.accepted)return;
 if(verdict.reset){authoritative=false;seamView.initialized=false;portalEpoch=0;portalAck=false;}
 if(next.portalEpoch>=portalEpoch){portalEpoch=next.portalEpoch;portalAck=next.portalOpen===true;portalAckAt=Date.now();}
 if(sample)near.update(next.rssi,Date.now());
 if(authoritative&&state&&next.revision<state.revision)return;
 authoritative=true;
 const before=animals();state=next;seamView.apply(next.seam,next.serverMs,next._clockLocal??performance.now()/1000);motion.renderOffset=seamView.offset;
 for(const f of animals())if(f.owner==='phone'&&before.find(old=>old.id===f.id)?.owner!=='phone'){motion.enter(f,width,height);Object.assign(positions[f.id],{x:f.id==='crab'?1:0,y:f.entryY||.5});}
 store.set('reef-state',JSON.stringify(next));link.received(Date.now());online=true;if(['正在连接鱼缸…','连接稍有延迟，正在重试…','连接中断，跨屏已停止；已经过去的伙伴留在那边。'].includes($('feedback').textContent))say('');render();
}
function render(){const a=animals(),desk=a.filter(f=>f.owner==='tank').length,phone=a.filter(f=>f.owner==='phone').length;
 $('deskCount').textContent=state?desk:'—';$('phoneCount').textContent=state?phone:'—';$('tankCount').textContent=state?`${phone} 位伙伴${seamView.active()&&seamView.state.toPhone?(seamView.progress(performance.now()/1000)<0?' · 1位正靠近':' · 1位正在进来'):''}`:'';
 $('tankLabel').textContent='手机这一边';$('connection').textContent=link.status(Date.now())==='retrying'?'连接重试中':online?'鱼缸已连接':state?'离线陪伴':'尚未连接';$('connection').className='status'+(online?'':' off');
 $('tankNote').textContent='点伙伴转身 · 点空白投食';$('autoToggle').textContent=autoEnabled?'暂停跨屏':'开启跨屏';
 const crossing=seamView.active()?seamView.state:null;
 $('proximityLabel').textContent=!online?'连接已断开，穿越暂停，重连后确认':crossing?(crossing.phase==='rollback'?'正在退回原来的鱼缸':seamView.progress(performance.now()/1000)<0?'正在靠近接缝…':'正在穿过接缝…'):!autoEnabled?'通道暂停，伙伴留在各自鱼缸':near.filtered===null?'正在读取远近信号…':canCross()?'通道已打开：谁碰到边缘，谁就跨过去':near.near?'连接确认中，跨屏暂缓':'通道已关闭：远近信号尚未稳定';
 $('fishList').replaceChildren(...a.map(f=>{const card=document.createElement('div');card.className='fish-card';const icon=document.createElement('div');icon.className='ascii';icon.style.color=f.color;icon.style.fontSize=f.scale===2?'20px':'14px';icon.textContent=f.id==='crab'?'Y(oo)Y':f.id==='octopus'?'(~~~)':'><(((o>';const name=document.createElement('div');name.className='fish-name';name.textContent=names[f.id];const where=document.createElement('div');where.className='fish-location';where.textContent=seamView.active()&&seamView.state.fish===f.id?(seamView.state.toPhone?'正在去手机':'正在回桌面'):f.owner==='tank'?'在桌面鱼缸':f.owner==='phone'?'在手机鱼缸':'在另一部手机';card.append(icon,name,where);return card}));
}
function clearPending(){store.remove('reef-seam-pending')}
function reconcile(){const request=pending();if(!request)return;const f=animals().find(f=>f.id===request.fish),s=seamView.state;
 if(!f||f.owner!=='phone'||(s&&s.op===request.op)){clearPending();return;}
 if(!canCross()||Number(request.epoch)!==portalEpoch){clearPending();if(positions[request.fish]){motion.cancel(request.fish);positions[request.fish].x=request.fish==='crab'?1:0;}}
}
async function refresh(){if(refreshing)return;refreshing=true;try{
 // One request both renews the lease and samples RSSI; no sequential state GET.
 const desired=autoEnabled&&near.near&&near.fresh(Date.now())&&!document.hidden;
 const response=await api('/api/portal',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({client,near:desired?'1':'0'})});
 apply(response);reconcile();handleSeam();
 }catch{link.failed();online=link.live(Date.now());if(!online){portalAck=false;reconcile();say('连接中断，跨屏已停止；已经过去的伙伴留在那边。')}else say('连接稍有延迟，正在重试…');render();}
 finally{refreshing=false;}}
async function sendSeam(request){
 if(seamSending||Date.now()<seamRetry)return;seamSending=true;
 try{const response=await api('/api/seam',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({...request,client})});apply(response,false);clearPending();}
 catch(e){if(e.state?.creatures)apply(e.state,false);seamRetry=Date.now()+500;if(e.status===423||e.status===409){clearPending();const p=positions[request.fish];if(p&&!p.seamControlled){p.dir=1;p.seamPreparing=false;p.seamWaiting=false;p.edgeExit=false;p.next=performance.now()/1000+3;}}if(e.status===412||e.status===400){clearPending();const p=positions[request.fish];if(p){motion.cancel(request.fish);p.x=request.fish==='crab'?1:0;}}}
 finally{seamSending=false;}
}
function returnAtEdge(id){
 if(!canCross()||seamSending||Date.now()<seamRetry)return;const f=animals().find(f=>f.id===id),p=positions[id];if(!f||f.owner!=='phone'||!(p?.seamPreparing||p?.seamWaiting))return;
 let request=pending();if(!request||request.fish!==id)request={fish:id,action:'begin',edge:'left',left:motion.bounds(f,p,height).left/width,height:p.y,speed:p.vx,epoch:portalEpoch,op:'seam-'+Date.now().toString(36)+'-'+Math.random().toString(36).slice(2)};
 request={...request,left:motion.bounds(f,p,height).left/width,height:p.y,speed:p.vx};
 const source={...motion.bounds(f,p,height),left:0};const blocked=animals().filter(other=>other.owner==='phone'&&other.id!==id).some(other=>{const q=positions[other.id];if(!q)return false;const b=motion.bounds(other,q,height);return source.left<b.right+6&&source.right+6>b.left&&source.top<b.bottom+6&&source.bottom+6>b.top;});if(blocked){p.dir=1;p.seamPreparing=false;p.seamWaiting=false;p.edgeExit=false;p.next=performance.now()/1000+3;return;}
 store.set('reef-seam-pending',JSON.stringify(request));sendSeam(request);
}
function handleSeam(){const s=seamView.state;if(!s||s.phase!=='offer'||!s.toPhone||!canCross()||seamSending)return;
 const f=animals().find(f=>f.id===s.fish);if(!f)return;const pose={...seamView.pose(performance.now()/1000),x:f.id==='crab'?1:0};
 const residents=animals().filter(f=>f.owner==='phone');if(motion.room(f,pose,pose.x,pose.y,residents,width,height))sendSeam({action:'accept',fish:s.fish,op:s.op,epoch:s.epoch});
}
function seamFrame(t,list){const s=seamView.state;motion.seamReservation=null;
 // Freeze uncertain crossings until authoritative rollback/completion arrives.
 // Never invent an ownership rollback after a lost completion response.
 if(seamView.active()&&!canCross()&&s.phase!=='rollback'&&seamView.freezeProgress===null)seamView.freezeProgress=seamView.progress(t);
 const active=seamView.active();
 for(const f of animals()){const p=positions[f.id];if(!p?.seamControlled)continue;
   if(!active||s.fish!==f.id){p.seamControlled=false;p.seamWaiting=false;p.seamPreparing=false;p.crossed=false;p.edgeExit=false;p.next=0;
     if(f.owner==='phone'){p.x=(s?.phase==='cancelled'?s.lead||0:0)+(f.id==='crab'?1:0);p.y=s?.height||p.y;p.dir=1;p.entering=true;p.vx=f.id==='crab'?20:17;p.vy=0;}
   }
 }
 if(!active)return list;
 const f=animals().find(f=>f.id===s.fish);if(!f)return list;
 if(s.phase==='rollback'&&seamView.progress(t)<=seamView.origin()+.00001){s.phase='cancelled';return seamFrame(t,list);}
 if(!s.toPhone||s.phase!=='offer'||seamSending){const pose={...seamView.pose(t),x:f.id==='crab'?1:0},box=motion.bounds(f,pose,height);if(!s.toPhone)box.right+=s.lead||0;motion.seamReservation={id:f.id,...box};}
 Object.assign(positions[f.id]||(positions[f.id]={}),seamView.pose(t),{seamWaiting:false,seamOperation:s.op});
 if(s.toPhone&&f.owner==='tank')return [...list,{...f,preview:true}];
 return list;
}
function closePortal(){portalAck=false;near.disarm();near.filtered=null;const body=new URLSearchParams({client,near:'0'});fetch('/api/portal',{method:'POST',body,keepalive:true}).catch(()=>{});render();}
$('retry').onclick=refresh;$('calibrateNear').onclick=()=>{if(near.filtered===null)return;const value=Math.round(near.filtered)-2;closePortal();near.threshold=value;store.set('reef-near-threshold',String(value));say('已记录这个摆放位置。拿开后通道关闭，再靠近时自动开启。');refresh()};$('resetNear').onclick=()=>{closePortal();near.threshold=null;store.remove('reef-near-threshold');say('已恢复相对信号判断，请移远后再靠近。');refresh()};$('autoToggle').onclick=()=>{autoEnabled=!autoEnabled;store.set('reef-auto',autoEnabled?'on':'off');closePortal();refresh()};
function resize(){const dpr=devicePixelRatio||1;canvas.width=Math.round(canvas.clientWidth*dpr);canvas.height=Math.round(canvas.clientHeight*dpr);ctx.setTransform(canvas.width/240,0,0,canvas.height/240,0,0);ctx.imageSmoothingEnabled=false;}new ResizeObserver(resize).observe(canvas);resize();
function restoreResidents(){let saved={};try{saved=JSON.parse(store.get('reef-positions-v24')||'{}')}catch{}const list=animals().filter(f=>f.owner==='phone'),firstDirection=Math.random()<.5?-1:1;let residentIndex=0;
 for(const f of list){let p=saved[f.id];if(p&&Number.isFinite(p.x)&&Number.isFinite(p.y)&&!p.crossed){positions[f.id]={x:Math.max(12,Math.min(width-span(f)-12,p.x*width)),y:p.y,dir:p.dir===-1?-1:1,vx:Number.isFinite(p.vx)?Math.abs(p.vx):17,vy:Number.isFinite(p.vy)?p.vy:0,entering:false,next:0,crossed:false};continue;}
  for(let trial=0;trial<50;trial++){const y=f.id==='crab'?220/240:f.id==='large'?.6:f.id==='small'?.3:.75,x=12+Math.random()*(width-span(f)-24),dir=(residentIndex%2? -firstDirection:firstDirection),candidate={x,y,dir,entering:false,vx:17,vy:0,next:0,crossed:false};if(motion.room(f,candidate,x,y,list,width,height)||trial===49){positions[f.id]=candidate;residentIndex++;break}}
 }}
function savePositions(){const saved={};for(const [id,p] of Object.entries(positions))saved[id]={x:p.x/width,y:p.y,dir:p.dir,vx:p.vx,vy:p.vy};store.set('reef-positions-v24',JSON.stringify(saved))}
function screenInk(color){const n=parseInt(color.slice(1),16),r=(n>>16)&255,g=(n>>8)&255,b=n&255;return `rgb(${Math.round((r>>3)*255/31)},${Math.round((g>>2)*255/63)},${Math.round((b>>3)*255/31)})`}
function pixel(x,y,color){ctx.fillStyle=screenInk(color);ctx.fillRect(Math.trunc(x),Math.trunc(y),1,1)}
function line(x0,y0,x1,y1,color){x0=Math.trunc(x0);y0=Math.trunc(y0);x1=Math.trunc(x1);y1=Math.trunc(y1);let dx=Math.abs(x1-x0),sx=x0<x1?1:-1,dy=-Math.abs(y1-y0),sy=y0<y1?1:-1,error=dx+dy;while(true){pixel(x0,y0,color);if(x0===x1&&y0===y1)break;let e=2*error;if(e>=dy){error+=dy;x0+=sx}if(e<=dx){error+=dx;y0+=sy}}}
function glyph(ch,x,y,size,color){const columns=REEF_GLYPHS[ch]||REEF_GLYPHS[' '];ctx.fillStyle=screenInk(color);for(let c=0;c<5;c++)for(let row=0;row<8;row++)if(columns[c]&(1<<row))ctx.fillRect(Math.trunc(x)+c*size,Math.trunc(y)+row*size,size,size)}
function textPixels(text,x,y,size,color){for(let c=0;c<text.length;c++)glyph(text[c],x+c*6*size,y,size,color)}
function drawCreature(f,p,t){
 const x=p.x,phase=f.id==='large'?.1:4.8;
 let base=p.y*height;
 if(f.id==='large'||f.id==='small'){
  base+=Math.sin(t*.7+phase)*5+Math.sin(t*1.4+phase*1.3)*1.5;
  const scale=f.id==='large'?2:1,body=p.dir>0?'><(((o>':'<o)))><';
  for(let c=0;c<7;c++){const from=p.dir>0?6-c:c,wave=Math.sin(t*4-from*.8+phase)*(.4+from*.32);const ch=t<(p.biteUntil||0)&&c===(p.dir>0?6:0)?(p.dir>0?')':'('):body[c];glyph(ch,x+c*6*scale,base+wave,scale,body[c]==='o'?'#e2d98b':from>=5?'#c3ded6':f.color)}
  const finX=Math.trunc(x)+7*6*scale/2,finY=Math.trunc(base+Math.sin(t*4-3*.8+phase)*1.36)+7*scale;
  line(finX,finY,finX+(Math.sin(t*5+phase)>0?2:-2),finY+2,'#c3ded6');
 }else if(f.id==='crab'){
  const cx=Math.trunc(x),cy=Math.trunc(base),claws=Math.sin(t*4.5)>0?'Y(oo)Y':'>(oo)<';textPixels(claws,cx,cy,1,'#ffffff');
  for(let leg=0;leg<3;leg++){const step=Math.sin(t*6+leg*2)>0?1:-1,root=cx+9+leg*6;line(root,cy+8,root-3+step,cy+12,'#ffffff');line(root+2,cy+8,root+4+step,cy+13,'#ffffff')}
 }else{
  const cx=Math.trunc(x+18),cy=Math.trunc(base+Math.sin(t*1.1)*2);textPixels(' ___ ',cx-15,cy-12,1,'#90cbd4');textPixels('(~~~)',cx-15,cy-4,1,'#90cbd4');
  for(let arm=0;arm<4;arm++){const root=cx-9+arm*6;let px=root,py=cy+4;for(let seg=1;seg<=4;seg++){const ax=root+Math.trunc(Math.sin(t*2.2-seg*.7+arm)*seg*.45),ay=cy+4+seg*2;line(px,py,ax,ay,'#c4ddda');px=ax;py=ay}}
 }
 const bx=f.id==='large'||f.id==='small'?(p.dir>0?x+span(f)-2*(f.id==='large'?2:1):x):f.id==='crab'?x+17:x+34;
 if(bx>0&&bx<width&&!motion.feeding){ctx.beginPath();ctx.arc(bx,base-((t*15)%48),2,0,Math.PI*2);ctx.strokeStyle='#a3c4cb';ctx.stroke()}
}
function drawPlants(t){
 const roots=[15,32,62,164,199,220];
 for(let i=0;i<6;i++){const height=46+(i*17)%44;let px=roots[i],py=228;for(let h=3;h<=height;h+=3){const sway=Math.sin(h*.24+t*1.1+i)*(.7+h*.035),x=roots[i]+Math.trunc(sway),y=228-h;line(px,py,x,y,i%3?'#2d5e4c':'#274c48');if(h%12===0)line(x,y,x+((h/12)%2?3:-3),y-4,'#2d5e4c');px=x;py=y;}}
 for(let i=0;i<36;i++)pixel((i*37+11)%240,229+(i*7)%10,i%3?'#212021':'#424142');
}
function draw(ms){const t=ms/1000,sceneT=t+seamView.offset,dt=Math.min((ms-previous)/1000||0,.1);previous=ms;ctx.fillStyle='#000000';ctx.fillRect(0,0,width,height);drawPlants(sceneT);ctx.lineWidth=1;for(let i=0;i<3;i++){ctx.beginPath();ctx.arc((i*59+31)%width,height-((t*(8+i%3*3)+i*37)%(height-35)),1+i%2,0,Math.PI*2);ctx.strokeStyle='#456575';ctx.stroke()}const gate=canCross();if(gate!==lastCrossState){lastCrossState=gate;render();}
 const list=seamFrame(t,animals().filter(f=>f.owner==='phone')),task=pending();
 const events=motion.step(list,dt,t,width,height,canCross(),task?.fish);
 for(const f of list){const p=positions[f.id];if(p&&!p.crossed)drawCreature(f,{...p,biteUntil:p.biteUntil?p.biteUntil+seamView.offset:0},sceneT)}
 for(const food of motion.food){ctx.globalAlpha=Math.max(0,Math.min(1,(45-food.age)/3));pixel(food.x,food.y,'#e2d98b');if(!food.swallow)pixel(food.x+1,food.y+1,'#e2d98b');}ctx.globalAlpha=1;
 for(const effect of motion.effects){const age=(t-effect.t)/.45,d=3+age*5;line(effect.x-effect.dir*d,effect.y-2,effect.x-effect.dir*(d+2),effect.y-3,'#52605c');}
 if(events.length)returnAtEdge(events[0]);
 requestAnimationFrame(draw)}
let press=null;
canvas.style.touchAction='manipulation';
canvas.addEventListener('pointerdown',e=>{press={x:e.clientX,y:e.clientY};});
canvas.addEventListener('pointerup',e=>{if(!press||Math.hypot(e.clientX-press.x,e.clientY-press.y)>10){press=null;return;}press=null;const box=canvas.getBoundingClientRect(),x=(e.clientX-box.left)*240/box.width,y=(e.clientY-box.top)*240/box.height,t=performance.now()/1000,list=animals().filter(f=>f.owner==='phone'),hit=motion.hit(list,x,y,height);if(hit){motion.turn(hit.id,t);say(`${names[hit.id]}转身了。`)}else if(list.length){motion.feed(x,y,list,t,width,height);say('撒下一小撮，慢慢吃。')}else say('手机鱼缸还没有伙伴，先接几位过来。');});
canvas.addEventListener('pointercancel',()=>{press=null;});
restoreResidents();render();refresh();requestAnimationFrame(draw);setInterval(()=>{if(seamView.active()||Date.now()-(lastPoll)>260){lastPoll=Date.now();refresh()}},100);setInterval(savePositions,3000);
addEventListener('online',refresh);addEventListener('pagehide',()=>{savePositions();closePortal()});document.addEventListener('visibilitychange',()=>{if(document.hidden){savePositions();closePortal()}else refresh()});
})();
