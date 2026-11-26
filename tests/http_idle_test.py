"""Compile the real server wrapper against a controlled single-socket server."""
from pathlib import Path
import subprocess, tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp)
    (p/'WebServer.h').write_text('''#pragma once
#include <stdint.h>
extern uint32_t clockMs;inline uint32_t millis(){return clockMs;}
enum HTTPClientStatus{HC_NONE,HC_WAIT_READ};
struct Client{bool bytes=false,stopped=false;int available(){return bytes;}void stop(){stopped=true;}};
class WebServer{public:explicit WebServer(int){};virtual void handleClient(){if(_currentStatus==HC_NONE&&queued){queued=false;processed++;}else if(_currentStatus==HC_WAIT_READ&&_currentClient.bytes){processed++;_currentStatus=HC_NONE;}}
bool queued=false;int processed=0;
protected:HTTPClientStatus _currentStatus=HC_NONE;uint32_t _statusChange=0;Client _currentClient;};
''')
    (p/'test.cpp').write_text('''#include "ReefWebServer.h"
#include <cassert>
#include <cstdio>
uint32_t clockMs=0;
struct Harness:ReefWebServer{Harness():ReefWebServer(80){}void socket(bool bytes,uint32_t start){_currentStatus=HC_WAIT_READ;_statusChange=start;_currentClient={bytes,false};}bool stopped(){return _currentClient.stopped;}};
int main(){Harness s;s.socket(false,100);s.queued=true;clockMs=400;s.handleClient();assert(s.processed==0&&s.idleDrops==0);
clockMs=451;s.handleClient();assert(s.processed==1&&s.idleDrops==1&&s.stopped());
s.socket(true,500);clockMs=5000;s.handleClient();assert(s.processed==2&&s.idleDrops==1&&!s.stopped());
s.socket(false,0xfffffff0);clockMs=400;s.handleClient();assert(s.idleDrops==2);
puts("PASS actual server wrapper: idle socket releases queued heartbeat in351ms, request bytes preserved, millis wrap safe");}
''')
    subprocess.run(['c++','-std=c++17','-I'+str(p),'-I'+str(root/'AquariumHome'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
