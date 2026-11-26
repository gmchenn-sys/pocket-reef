class NearApproach {
  constructor(){this.threshold=null;this.disarm();this.filtered=null;}
  disarm(){this.near=false;this.far=null;this.nearSince=null;this.leaveSince=null;this.samples=[];this.lastValid=0;this.reason='learning';this.closeLevel=-55;}
  update(raw,now){
    if(!Number.isFinite(raw)||raw>10||raw< -110){this.reason='no-signal';return this.near;}
    this.lastValid=now;this.samples.push(raw);if(this.samples.length>5)this.samples.shift();
    const sorted=this.samples.slice().sort((a,b)=>a-b),median=sorted[Math.floor(sorted.length/2)];
    this.filtered=this.filtered===null?median:this.filtered*.65+median*.35;
    this.far=this.far===null?this.filtered:Math.min(this.far,this.filtered);
    // Saturated close-range RSSI (including zero) must not set an unrealistically
    // strict exit threshold. Hand/orientation changes can vary many dB nearby.
    const enter=this.threshold===null?Math.min(-45,this.far+8):Math.min(-45,this.threshold);
    const leave=this.closeLevel;
    if(this.near){
      if(this.filtered<leave){if(this.leaveSince===null)this.leaveSince=now;if(now-this.leaveSince>=1000){this.near=false;this.nearSince=null;this.leaveSince=null;this.reason='far';}}
      else{this.leaveSince=null;this.reason='near';}
    }else if(this.filtered>=enter){
      if(this.nearSince===null)this.nearSince=now;
      if(now-this.nearSince>=1400){this.near=true;this.closeLevel=Math.min(-55,Math.min(-45,this.filtered)-8);this.leaveSince=null;this.reason='near';}
    }else{this.nearSince=null;this.reason='far';}
    return this.near;
  }
  fresh(now){return this.lastValid>0&&now-this.lastValid<ReefLink.FRESH_MS;}
}
// A missed heartbeat is not evidence that the phone moved away. Bound the
// grace period to the device lease; valid weak RSSI still closes independently.
class ReefLink {
  static FRESH_MS=6000;
  static REQUEST_MS=1800;
  constructor(){this.lastGood=null;this.failures=0;}
  received(now){this.lastGood=now;this.failures=0;}
  failed(){this.failures++;}
  live(now){return this.lastGood!==null&&now-this.lastGood<ReefLink.FRESH_MS;}
  status(now){return !this.live(now)?'offline':this.failures||now-this.lastGood>1200?'retrying':'connected';}
}
if(typeof module!=='undefined')module.exports=NearApproach;
if(typeof module!=='undefined')module.exports.ReefLink=ReefLink;
