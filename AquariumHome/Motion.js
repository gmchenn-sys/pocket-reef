// Same 240x240 motion model as updateSwimmers/updateBottom on the ST7789.
class MobileWander {
  constructor(random=Math.random){this.random=random;this.positions={};this.food=[];this.feeding=null;this.eaten={};this.consumed=[];this.effects=[];this.foodId=0;this.seamEnabled=false;this.renderOffset=0;}
  pick(a,b){return a+Math.floor(this.random()*(b-a))}
  hit(list,x,y,h){return [...list].reverse().find(f=>{const p=this.positions[f.id];if(!p||p.crossed||p.seamControlled||p.seamWaiting)return false;const b=this.bounds(f,p,h);return x>=b.left-4&&x<=b.right+4&&y>=b.top-4&&y<=b.bottom+4;});}
  turn(id,t){const p=this.positions[id];if(!p||p.crossed||p.seamControlled||p.seamWaiting)return false;p.dir=-p.dir;p.edgeExit=false;p.entering=false;p.next=t+6;p.turnUntil=t+1.5;return true;}
  mouth(f,p,t,h){
    t+=this.renderOffset||0;
    if(f.id==='large'||f.id==='small'){const size=f.id==='large'?2:1,phase=f.id==='large'?.1:4.8;return{x:p.x+(p.dir>0?this.span(f)-2*size:0),y:p.y*h+Math.sin(t*.7+phase)*5+Math.sin(t*1.4+phase*1.3)*1.5+Math.sin(t*4+phase)*.4+3*size};}
    return{x:p.x+(f.id==='crab'?17:18),y:p.y*h+(f.id==='crab'?4:5)};
  }
  pickup(f,p,t,h){const mouth=this.mouth(f,p,t,h);if(f.id==='crab'||f.id==='octopus')return{x:p.x+(p.dir>0?34:2),y:mouth.y};return mouth;}
  feed(x,y,list,t,w,h){
    x=Math.max(14,Math.min(w-14,x));y=Math.max(16,Math.min(h-35,y));
    this.feeding={x,y,until:t+45};this.eaten={};
    for(let i=0;i<12;i++)this.food.push({id:++this.foodId,x:x+(this.random()-.5)*18,y:y+(this.random()-.5)*16,vx:(this.random()-.5)*22,sink:4+this.random()*7,phase:this.random()*6.28,age:0});
    this.food=this.food.slice(-48);
    for(const f of list){const p=this.positions[f.id];if(p&&!p.crossed){p.target=null;p.next=0;}}
  }
  particles(list,dt,t,w,h){
    const alive=[];
    for(const q of this.food){q.age+=dt;
      if(q.swallow){const f=list.find(f=>f.id===q.swallow),p=f&&this.positions[f.id];
        if(!p||p.crossed){q.swallow=null;q.claim=null;}
        else{const mouth=this.mouth(f,p,t,h),blend=1-Math.exp(-dt*22);q.x+=(mouth.x-q.x)*blend;q.y+=(mouth.y-q.y)*blend;
          if(t>=q.goneAt){this.eaten[f.id]=(this.eaten[f.id]||0)+1;this.consumed.push({id:q.id,by:f.id,t});p.biteUntil=t+.17;p.chewing=false;p.cooldown=t+.65+this.random()*.4;p.target=null;this.effects.push({x:mouth.x,y:mouth.y,t,dir:p.dir});continue;}}
      }
      if(!q.swallow){q.vx*=Math.exp(-dt*.75);q.x+= (q.vx+Math.sin(t*.65+q.phase)*2.5)*dt;q.y=Math.min(h-15,q.y+q.sink*dt);q.x=Math.max(8,Math.min(w-8,q.x));}
      if(q.age<45)alive.push(q);
    }
    this.food=alive;this.effects=this.effects.filter(e=>t-e.t<.45);
    if(!this.food.length)this.feeding=null;
  }
  targets(list,t,w,h){
    for(const q of this.food)q.claim=q.swallow||null;
    const eligible=list.filter(f=>{const p=this.positions[f.id];return p&&!p.crossed&&!p.seamControlled&&!p.seamWaiting&&!p.entering&&!p.edgeExit&&t>=(p.detourUntil||0);});
    const unfed=list.filter(f=>!this.eaten[f.id]&&!this.positions[f.id]?.crossed).length;this.waitingFor=unfed;
    for(const f of eligible.sort((a,b)=>(this.eaten[a.id]||0)-(this.eaten[b.id]||0))){const p=this.positions[f.id];
      if(t<(p.cooldown||0))continue;
      if(this.eaten[f.id]&&this.food.filter(q=>!q.swallow&&!q.claim).length<=unfed)continue;
      let q=this.food.find(q=>q.id===p.target&&!q.swallow&&!q.claim);
      if(q&&t-(p.targetAt||0)>6)q=null;
      if(!q){const free=this.food.filter(q=>!q.swallow&&!q.claim);if(this.eaten[f.id]&&free.length<=unfed)continue;const mouth=this.mouth(f,p,t,h);free.sort((a,b)=>Math.hypot(a.x-mouth.x,a.y-mouth.y)-Math.hypot(b.x-mouth.x,b.y-mouth.y));q=free[0];if(q){p.target=q.id;p.targetAt=t;p.targetDir=q.x<this.span(f)+12?-1:q.x>w-this.span(f)-12?1:q.x>p.x+this.span(f)/2?1:-1;}}
      if(q){q.claim=f.id;p.feedGoal=q;}else p.target=null;
    }
  }
  span(f){return f.id==='large'?84:f.id==='small'?42:f.id==='octopus'?36:38}
  enter(f,w,h){this.positions[f.id]={x:-this.span(f)-6,y:f.id==='crab'?220/240:Math.max(.1,Math.min(.8,f.entryY||.5)),dir:1,entering:true,vx:f.id==='crab'?20:17,vy:0,next:0,crossed:false};}
  cancel(id){const p=this.positions[id];if(!p)return;p.crossed=false;p.seamWaiting=false;p.seamPreparing=false;p.seamControlled=false;p.edgeExit=false;p.entering=true;p.dir=1;p.next=0;}
  bounds(f,p,h,x=p.x,y=p.y){return {left:x+(f.id==='crab'?-1:0),right:x+this.span(f)-(f.id==='crab'?1:0),top:f.id==='crab'?y*h:y*h+(f.id==='octopus'?-17:-10),bottom:f.id==='crab'?y*h+15:y*h+(f.id==='large'?27:f.id==='small'?19:17)};}
  room(f,p,x,y,list,w,h){const a=this.bounds(f,p,h,x,y);if(a.right<0||a.left>=w)return true;const reserve=this.seamReservation;if(reserve&&reserve.id!==f.id&&a.left<reserve.right+6&&a.right+6>reserve.left&&a.top<reserve.bottom+6&&a.bottom+6>reserve.top)return false;for(const other of list){if(other.id===f.id)continue;const q=this.positions[other.id];if(!q||q.crossed)continue;const b=this.bounds(other,q,h);if(b.right<0||b.left>=w)continue;if(a.left<b.right+6&&a.right+6>b.left&&a.top<b.bottom+6&&a.bottom+6>b.top)return false;}return true;}
  step(list,dt,t,w,h,open,pendingId=null){
    const current=new Set(list.map(f=>f.id));for(const id of Object.keys(this.positions))if(!current.has(id))delete this.positions[id];
    for(const f of list)if(!this.positions[f.id])this.enter(f,w,h);
    const desired={};
    this.particles(list,dt,t,w,h);
    const feeding=!!this.feeding&&this.food.length>0;
    for(const p of Object.values(this.positions)){p.feedGoal=null;p.feedDx=null;p.chewing=false;}
    // Departure wins over feeding, including on the frame an animal touches the edge.
    for(const f of list){const p=this.positions[f.id];if(!open)p.edgeExit=false;else if(!p.entering&&p.dir<0&&this.bounds(f,p,h,p.x-p.vx*dt).left<=0)p.edgeExit=true;}
    if(feeding)this.targets(list,t,w,h);
    for(const f of list){const p=this.positions[f.id];if(p.seamControlled||p.seamWaiting)continue;
      if(p.crossed){if(!open&&pendingId!==f.id)this.cancel(f.id);continue;}
      // Once a resident reaches the shared edge heading out, finish that exit.
      // Feeding and a scheduled random turn cannot pull a half-departed body back.
      if(!open)p.edgeExit=false;
      else if(!p.entering&&p.dir<0&&this.bounds(f,p,h,p.x-p.vx*dt).left<=0)p.edgeExit=true;
      const crab=f.id==='crab',oct=f.id==='octopus',speed=crab?20:f.id==='large'?17:f.id==='small'?15:13;
      if(!p.next||t>=p.next){
        // Pick a cruising heading only once per long leg. Never chase an X target
        // or reverse because avoidance temporarily blocked horizontal movement.
        if(!p.edgeExit&&!feeding&&!p.entering&&(p.next&&p.x>8&&p.x+this.span(f)<w-8)&&this.pick(0,3)===0)p.dir=-p.dir;
        p.vx=crab?this.pick(18,29):speed*(.85+this.pick(0,31)/100);
        let y=this.pick(oct?28:24,oct?203:183);if(f.id==='large')y=y*.4+145*.6;if(f.id==='small')y=y*.4+70*.6;
        p.ty=y/h;p.next=t+(crab?this.pick(3500,8000):this.pick(4500,10000))/1000;
      }
      desired[f.id]=crab?Math.max(-20,Math.min(20,(220-p.y*h)*.8)):Math.max(-8,Math.min(8,(p.ty-p.y)*h*.35));
      p.dining=false;
      const q=p.feedGoal;
      if(q&&!p.entering&&!p.edgeExit){
        if(t>=(p.turnUntil||0))p.dir=p.targetDir;
        const mouth=this.pickup(f,p,t,h),dx=q.x+Math.sin(q.phase)*.5-mouth.x,dy=q.y-mouth.y;
        p.feedDx=Math.max(-35,Math.min(35,dx*1.55));desired[f.id]=Math.max(-25,Math.min(25,dy*1.4));p.next=t+2;
        if(Math.hypot(dx,dy)<8&&t>=(p.cooldown||0)&&!q.swallow){q.swallow=f.id;q.goneAt=t+.14;p.chewing=true;p.biteUntil=t+.14;p.cooldown=t+.5;p.feedDx*=.25;}
      }
      if(feeding&&!p.feedGoal&&!p.edgeExit&&this.eaten[f.id]&&this.waitingFor&&this.food.length<=this.waitingFor+1){const fx=this.food.reduce((sum,q)=>sum+q.x,0)/this.food.length;p.feedDx=(p.x+this.span(f)/2>fx?1:-1)*22;desired[f.id]=crab?Math.max(-15,Math.min(15,(220-p.y*h)*.8)):-18;}
      if(feeding&&!p.edgeExit&&t<(p.detourUntil||0)){p.feedDx=p.detourDir*16;desired[f.id]=-22;p.dir=p.detourDir;}

    }
    for(let i=0;i<list.length;i++)for(let j=i+1;j<list.length;j++){
      const f=list[i],g=list[j],p=this.positions[f.id],q=this.positions[g.id];if(p.seamControlled||q.seamControlled||p.seamWaiting||q.seamWaiting)continue;
      if((!feeding&&(f.id==='crab'||g.id==='crab'))||p.crossed||q.crossed)continue;
      const a=this.bounds(f,p,h),b=this.bounds(g,q,h),delta=q.y*h+(g.id==='octopus'?0:8.5)-p.y*h-(f.id==='octopus'?0:8.5);
      const separation=feeding?(a.bottom-a.top+b.bottom-b.top)/2+8:57;
      if(a.left<b.right+52&&a.right+52>b.left&&Math.abs(delta)<separation){const push=(separation-Math.abs(delta))*.6,sign=delta>=0?1:-1;desired[f.id]-=sign*push;desired[g.id]+=sign*push;}
    }
    const events=[];
    // The hardware updates its bottom creature first.
    for(const f of [...list].sort((a,b)=>(b.id==='crab')-(a.id==='crab'))){const p=this.positions[f.id];if(p.seamControlled)continue;if(p.seamWaiting){if(open)events.push(f.id);else this.cancel(f.id);continue;}if(p.crossed){if(open)events.push(f.id);continue;}
      const crab=f.id==='crab';p.vy+=((Math.max(feeding?-25:-20,Math.min(feeding?25:20,desired[f.id]||0)))-p.vy)*(1-Math.exp(-dt*2));
      const normalMax=crab?220:f.id==='octopus'?202:182,limit=feeding?(crab?220:f.id==='large'?208:216):Math.max(normalMax,p.y*h);
      const y=Math.max(f.id==='octopus'?28:24,Math.min(limit,p.y*h+p.vy*dt))/h;
      let blocked=false;if(this.room(f,p,p.x,y,list,w,h))p.y=y;else{blocked=Math.abs(y-p.y)>.0001;p.vy=0;}
      if(p.edgeExit){p.dir=-1;p.dining=false;}
      const velocity=p.feedDx!==null&&!p.edgeExit?p.feedDx:p.dir*p.vx;
      let x=p.x+velocity*dt;
      if(this.seamEnabled&&!feeding&&!p.entering&&open&&p.dir<0&&this.bounds(f,p,h,x).left<=48){p.seamPreparing=true;events.push(f.id);}
      if(this.seamEnabled&&!p.entering&&open&&p.dir<0&&this.bounds(f,p,h,x).left<=1){p.x=f.id==='crab'?1:0;p.seamWaiting=true;p.edgeExit=true;events.push(f.id);continue;}
      if(!p.entering&&open&&p.dir<0&&this.bounds(f,p,h,x).right<=0){p.x=x;p.crossed=true;events.push(f.id);continue;}
      // Just as on hardware, fully leave and re-enter from the opposite edge.
      // A closed portal changes no ownership and never creates a wall turn.
      if(p.dir>0&&x>w+6)x=crab?-44:-this.span(f)-5;
      if(p.dir<0&&x<-(crab?44:this.span(f)+5))x=w+6;
      if(this.room(f,p,x,p.y,list,w,h))p.x=x;else if(Math.abs(x-p.x)>.01)blocked=true;
      if(feeding&&p.feedGoal&&!p.edgeExit){p.blockFor=blocked?(p.blockFor||0)+dt:Math.max(0,(p.blockFor||0)-dt*.5);if(p.blockFor>.7&&t>=(p.detourUntil||0)){p.detourUntil=t+1.6;p.detourDir=p.x+this.span(f)/2>this.feeding.x?1:-1;p.target=null;p.blockFor=0;}}
      if(p.entering&&p.x>=12){p.entering=false;}
    }
    return events;
  }
}
if(typeof module!=='undefined')module.exports=MobileWander;
