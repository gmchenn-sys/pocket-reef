import json, threading, urllib.parse, time
from pathlib import Path
from http.server import BaseHTTPRequestHandler, HTTPServer
ROOT=Path(__file__).resolve().parent
sequence=0
start=time.monotonic();owners=[None]*4;revision=0;ops={};signal=None;seam=None;blocked=False
ids=['large','small','octopus','crab'];widths=[84,42,36,38];heights=[.6,.3,.5,220/240]
portal={'open':False,'client':'','epoch':1,'until':0}
def ms():return int((time.monotonic()-start)*1000)
def active():return seam and seam['phase'] in ['offer','running','rollback']
def origin():return -seam.get('lead',0)/seam['width'] if seam else 0
def raw():return origin()+(ms()-seam['started'])/seam['duration']*(1-origin())
def progress():
    if not seam:return 0
    if seam['phase']=='done':return 1
    if seam['phase']=='running':return max(origin(),min(1,raw()))
    if seam['phase']=='offer' and seam.get('lead',0)>0:return max(origin(),min(-8/seam['width'],raw()))
    if seam['phase']=='rollback':
        t=max(0,min(1,(ms()-seam['started'])/seam.get('rollbackDuration',500)));return seam['rollbackFrom']+(origin()-seam['rollbackFrom'])*(t*t*(3-2*t))
    return origin()
def cancel():
    u=progress();distance=abs(u-origin())*seam['width'];duration=(1 if distance<1 else max(250,min(1600,distance/seam['speed']*1000))) if seam.get('lead',0)>0 else 500
    seam.update(phase='rollback',rollbackFrom=u,started=ms(),rollbackDuration=duration)
def offer(i,to_phone,client,op,lead=0,speed=None):
    global seam
    speed=speed or [17,15,13,20][i]
    seam={'fish':ids[i],'phase':'offer','toPhone':to_phone,'client':client,'op':op,'epoch':portal['epoch'],'height':heights[i],'width':widths[i],'lead':lead,'speed':speed,'started':ms(),'duration':(widths[i]+lead)/speed*1000 if lead else max(1800,min(4200,widths[i]/speed*1000)),'rollbackFrom':0,'rollbackDuration':500}
def run():
    if seam and seam['phase']=='offer':
        if seam.get('lead',0)>0:
            if raw()>=-8/seam['width']:cancel()
            else:seam['phase']='running'
        else:seam.update(phase='running',started=ms()+220)
def update():
    global revision
    if portal['open'] and time.monotonic()>portal['until']:portal['open']=False;portal['epoch']+=1
    if not active():return
    if seam['phase']!='rollback' and (not portal['open'] or seam['epoch']!=portal['epoch']):
        cancel()
    if seam['phase']=='offer' and seam.get('lead',0)>0 and raw()>=-8/seam['width']:cancel()
    if seam['phase']=='offer' and not seam['toPhone'] and not blocked:run()
    if seam['phase']=='running' and progress()>=1:
        i=ids.index(seam['fish']);owners[i]=seam['client'] if seam['toPhone'] else None;heights[i]=seam['height'];revision+=1;ops[seam['op']]=True;seam['phase']='done'
    if seam['phase']=='rollback' and progress()<=origin()+.00001:seam['phase']='cancelled'
class Handler(BaseHTTPRequestHandler):
    def reply(self,code,data,kind='application/json'):
        body=data if isinstance(data,bytes) else json.dumps(data).encode();self.send_response(code);self.send_header('Content-Type',kind);self.send_header('Cache-Control','no-store');self.end_headers();self.wfile.write(body)
    def state(self,client):
        global sequence
        sequence+=1;update()
        return {'boot':1,'seq':sequence,'serverMs':ms(),'revision':revision,'rssi':signal,'portalOpen':portal['open'] and portal['client']==client,'portalEpoch':portal['epoch'],'seam':seam if seam and seam['client']==client else None,'creatures':[
          {'id':ids[i],'scale':2 if i==0 else 1,'color':['#a9dcd7','#d0c5e8','#90cbd4','#ffffff'][i],'owner':'tank' if o is None else 'phone' if o==client else 'other','phase':'idle','direction':1,'entryY':heights[i]} for i,o in enumerate(owners)]}
    def do_GET(self):
        global signal,blocked
        u=urllib.parse.urlparse(self.path);q=urllib.parse.parse_qs(u.query);update()
        if u.path=='/':
            page=(ROOT/'index.html').read_text().replace('restoreResidents();render();refresh();','window.__reefTest={motion,animals,canCross,near,positions,seamView};restoreResidents();render();refresh();')
            self.reply(200,page.encode(),'text/html; charset=utf-8')
        elif u.path=='/api/state':self.reply(200,self.state(q.get('client',[''])[0]))
        elif u.path=='/test/signal':signal=int(q['value'][0]);self.reply(200,{'rssi':signal})
        elif u.path=='/test/block-target':blocked=q['value'][0]=='1';self.reply(200,{'blocked':blocked})
        elif u.path=='/test/desktop-edge':
            i=ids.index(q['id'][0])
            if not portal['open']:self.reply(412,{'error':'Edge closed'});return
            if active():self.reply(423,{'error':'Seam busy'});return
            if owners[i] is not None:self.reply(409,{'error':'Not desktop owned'});return
            offer(i,True,portal['client'],'mock-'+str(ms()),48);self.reply(200,self.state(portal['client']))
        elif u.path=='/test/pose':
            if not seam:self.reply(200,{});return
            w=seam['width'];u=progress();left=240-w+u*w if seam['toPhone'] else 240-u*w
            self.reply(200,{'progress':u,'desktopLeft':left,'phoneLeft':-w+u*w if seam['toPhone'] else -u*w,'owner':owners[ids.index(seam['fish'])]})
        else:self.reply(404,{'error':'Not found'})
    def do_POST(self):
        p={k:v[0] for k,v in urllib.parse.parse_qs(self.rfile.read(int(self.headers.get('Content-Length',0))).decode()).items()};client=p.get('client','')
        if self.path=='/api/portal':
            opening=p.get('near')=='1'
            if opening:
                if portal['open'] and portal['client']!=client:self.reply(409,self.state(client));return
                if not portal['open']:portal['epoch']+=1
                portal.update(open=True,client=client,until=time.monotonic()+6)
            elif portal['client']==client and portal['open']:portal['open']=False;portal['epoch']+=1
            self.reply(200,self.state(client));return
        update()
        if self.path!='/api/seam':self.reply(409,{'error':'Refresh phone'});return
        i=ids.index(p['fish']);op=p['op'];action=p['action']
        if not portal['open'] or int(p.get('epoch',0))!=portal['epoch'] or portal['client']!=client:self.reply(412,{**self.state(client),'error':'Edge closed'});return
        if action=='accept':
            if not seam or seam['fish']!=ids[i] or seam['op']!=op or not seam['toPhone']:self.reply(409,self.state(client));return
            run();self.reply(200,self.state(client));return
        if owners[i]!=client:self.reply(409,{**self.state(client),'error':'Ownership changed'});return
        if active():
            self.reply(200 if seam['op']==op and seam['fish']==ids[i] else 423,self.state(client));return
        if p.get('edge')!='left' or float(p.get('left',1))>.26:self.reply(409,self.state(client));return
        offer(i,False,client,op,max(0,float(p['left'])*240),max(8,float(p.get('speed',20))));seam['height']=float(p['height']);update();self.reply(200,self.state(client))
server=HTTPServer(('127.0.0.1',8765),Handler);threading.Timer(600,server.shutdown).start()
print('PREVIEW_READY http://127.0.0.1:8765',flush=True);server.serve_forever()
