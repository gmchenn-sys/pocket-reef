class SnapshotOrder {
 constructor(){this.boot=null;this.last=-1;this.retired=new Set();}
 accept(snapshot){const boot=snapshot.boot??'legacy',seq=snapshot.seq??snapshot.serverMs??0;
  if(this.retired.has(boot))return {accepted:false,reset:false};
  const reset=this.boot!==boot;if(reset){if(this.boot!==null)this.retired.add(this.boot);this.boot=boot;this.last=-1;}
  if(seq<this.last)return {accepted:false,reset:false};this.last=seq;return {accepted:true,reset};
 }
}
// The two canvases clip complementary parts of this single body coordinate.
class SeamView {
  constructor(){this.state=null;this.offset=0;this.initialized=false;this.freezeProgress=null;}
  apply(value,serverMs,localNow){
    if(Number.isFinite(serverMs)){const offset=serverMs/1000-localNow;this.offset=this.initialized?this.offset*.8+offset*.2:offset;this.initialized=true;}
    this.state=value?.fish?value:null;this.freezeProgress=null;
  }
  active(){return this.state&&['offer','running','rollback'].includes(this.state.phase);}
  origin(){return -(this.state?.lead||0)/(this.state?.width||1);}
  progress(localNow){const s=this.state;if(!s)return 0;if(this.freezeProgress!==null)return this.freezeProgress;if(s.phase==='done')return 1;
    const elapsed=(localNow+this.offset)*1000-s.started,origin=this.origin();
    if(s.phase==='running')return Math.max(origin,Math.min(1,origin+elapsed/s.duration*(1-origin)));
    if(s.phase==='offer'&&s.lead>0)return Math.max(origin,Math.min(-8/s.width,origin+elapsed/s.duration*(1-origin)));
    if(s.phase==='rollback'){const t=Math.max(0,Math.min(1,elapsed/(s.rollbackDuration||500))),e=t*t*(3-2*t);return s.rollbackFrom+(origin-s.rollbackFrom)*e;}
    return origin;
  }
  pose(localNow,phone=true){const s=this.state,u=this.progress(localNow),w=s.width;
    const left=s.toPhone?(phone?-w+u*w:240-w+u*w):(phone?-u*w:240-u*w);
    return {x:left+(s.fish==='crab'?1:!phone&&s.fish==='octopus'?18:0),y:s.height,dir:s.phase==='rollback'?(s.toPhone?-1:1):s.toPhone?1:-1,vx:0,vy:0,seamControlled:true,entering:false,crossed:false};
  }
}
SeamView.SnapshotOrder=SnapshotOrder;
if(typeof module!=='undefined')module.exports=SeamView;
