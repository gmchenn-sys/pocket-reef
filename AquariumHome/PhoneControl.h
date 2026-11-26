#pragma once
#include <WiFi.h>
#include <esp_random.h>
#include <WebServer.h>
#include "ReefWebServer.h"
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_wifi_ap_get_sta_list.h>
#include "Ownership.h"
#include "Portal.h"
#include "Seam.h"
#include "WebPage.h"

const char *AP_SSID = "PocketReef";
const char *AP_PASSWORD = "Reef-258614";
ReefWebServer server(80);
Preferences preferences;
Ownership ownership = {};
bool networkReady = false, networkWanted = false, radioTrial = false;
uint32_t bootToken=0,stateSequence=0;
uint32_t radioStarted = 0, buttonSince = 0;
String serialCommand;
uint32_t httpRequests=0,lastHttpAt=0,rssiMisses=0,rssiSingleFallbacks=0;

EdgePortal portal;
SeamTransit seam;
const char *creatureIds[] = {"large", "small", "octopus", "crab"};
int creatureIndex(const String &id) {
  for (int i = 0; i < CREATURE_COUNT; ++i) if (id == creatureIds[i]) return i;
  return -1;
}
bool fishInTank(int index) { return index >= CREATURE_COUNT || !ownership.owner[index][0]; }
bool seamControlled(int i){return seam.active()&&seam.fish==i;}
bool fishVisible(int i){return fishInTank(i)||seamControlled(i);}
const char* seamPhase(){const char* names[]={"none","offer","running","rollback","done","cancelled"};return names[int(seam.phase)];}
bool seamBlocks(int i,float x,float y){
  if(!seam.active()||(!seam.toPhone&&seam.phase==SeamTransit::Offer)||seam.fish==i)return false;
  int j=seam.fish;float l=x-(i==2?18:i==3?1:0),r=l+SeamTransit::width(i),top=y-(i==2?17:i==3?0:10),bottom=y+(i==0?27:i==1?19:i==2?17:15);
  float sl=W-SeamTransit::width(j)-(seam.toPhone?seam.lead:0),sy=seam.height*H,st=sy-(j==2?17:j==3?0:10),sb=sy+(j==0?27:j==1?19:j==2?17:15);
  return l<W+6&&r+6>sl&&top<sb+6&&bottom+6>st;
}
int requestRssi() {
  wifi_sta_list_t list = {};
  if (!networkReady || esp_wifi_ap_get_sta_list(&list) != ESP_OK) return -127;
  wifi_sta_mac_ip_list_t addresses = {};
  IPAddress peer = server.client().remoteIP();
  if (esp_wifi_ap_get_sta_list_with_ip(&list, &addresses) == ESP_OK) {
    for (int a = 0; a < addresses.num; ++a) if (IPAddress(addresses.sta[a].ip.addr) == peer)
      for (int j = 0; j < list.num; ++j) if (!memcmp(addresses.sta[a].mac, list.sta[j].mac, 6)) return list.sta[j].rssi;
  }
  // DHCP/MAC lookup can briefly lag association. A sole AP station is
  // unambiguous; never substitute another phone's RSSI when two are connected.
  if (list.num == 1) { ++rssiSingleFallbacks; return list.sta[0].rssi; }
  ++rssiMisses;
  return -127;
}
void sendState(const String &client, int status = 200, const char *error = nullptr) {
  ++httpRequests;lastHttpAt=millis();
  int rssi = requestRssi();
  String result = "{\"boot\":"+String(bootToken)+",\"seq\":"+String(++stateSequence)+",\"serverMs\":"+String(millis())+",\"revision\":" + String(ownership.revision) + ",\"rssi\":" +
    (rssi == -127 ? String("null") : String(rssi)) + ",\"portalOpen\":" +
    (portal.allows(client.c_str(), portal.epoch, millis()) ? "true" : "false") +
    ",\"portalEpoch\":" + String(portal.epoch) + ",\"seam\":";
  if(seam.phase!=SeamTransit::None && client==seam.client){
    result+="{\"fish\":\""+String(creatureIds[seam.fish])+"\",\"phase\":\""+String(seamPhase())+"\",\"toPhone\":"+(seam.toPhone?"true":"false")+
      ",\"op\":\""+String(seam.operation)+"\",\"epoch\":"+String(seam.epoch)+",\"height\":"+String(seam.height,5)+",\"width\":"+String(SeamTransit::width(seam.fish))+
      ",\"lead\":"+String(seam.lead,5)+",\"rollbackDuration\":"+String(seam.rollbackDuration)+",\"started\":"+String(seam.started)+",\"duration\":"+String(seam.duration)+",\"rollbackFrom\":"+String(seam.rollbackFrom,5)+"}";
  }else result+="null";
  result+=",\"creatures\":[";
  for (int i = 0; i < CREATURE_COUNT; ++i) {
    if (i) result += ',';
    const char *owner = !ownership.owner[i][0] ? "tank" : client == ownership.owner[i] ? "phone" : "other";
    const char *colors[] = {"#a9dcd7", "#d0c5e8", "#90cbd4", "#ffffff"};
    result += "{\"id\":\"" + String(creatureIds[i]) + "\",\"scale\":" + String(i == 0 ? 2 : 1) +
      ",\"color\":\"" + colors[i] + "\",\"owner\":\"" + owner + "\",\"phase\":\"idle\",\"direction\":" +
      String(ownership.entryDirection[i] ? ownership.entryDirection[i] : 1) +
      ",\"entryY\":" + String(ownership.entryHeight[i] > 0 ? ownership.entryHeight[i] : (i == 0 ? .6f : i == 1 ? .3f : i == 3 ? .86f : .5f), 3) + "}";
  }
  result += "]";
  if (error) result += ",\"error\":\"" + String(error) + "\"";
  result += '}';
  server.sendHeader("Cache-Control", "no-store");
  server.send(status, "application/json; charset=utf-8", result);
}
TransferResult commitTransfer(int index, const String &client, const String &op, bool toPhone, float height = -1, bool joined = false) {
  Ownership next = ownership;
  TransferResult result = applyTransfer(next, index, client.c_str(), op.c_str(), toPhone);
  if (result != TransferResult::Changed) return result;
  next.entryDirection[index] = toPhone ? 1 : -1;
  if (toPhone) height = index == 3 ? crabY/H : swimmers[index].y / H;
  if (height < 0) height = index == 0 ? .6f : index == 1 ? .3f : .5f;
  next.entryHeight[index] = fmaxf(.1f,fminf(index==3?220.f/H:216.f/H,height));
  if (preferences.putBytes("fish", &next, sizeof(next)) != sizeof(next)) return TransferResult::Invalid;
  ownership = next;
  if (!toPhone) {
    if (index < 3) {
      swimmers[index].direction = -1; swimmers[index].x = joined? W-SeamTransit::width(index)+(index==2?18:0) : index == 2 ? W + 24 : W + 6;
      swimmers[index].y = fmaxf(24, fminf(216, height * H));
      swimmers[index].vy = 0; swimmers[index].next = 0;
    } else { crabDirection = -1; crabX = joined?W-37:W+6;crabY=next.entryHeight[index]*H;nextCrabChoice=0; }
  }
  Serial.printf("PHONE_EDGE_TRANSFER creature=%s target=%s revision=%lu\n", creatureIds[index],
                toPhone ? "phone" : "tank", (unsigned long)ownership.revision);
  return result;
}
void placeSeam(uint32_t now){
  if(!seam.active())return;
  float x=seam.anchor(false,now),y=seam.height*H;
  if(seam.fish==3){crabX=x;crabY=y;crabDirection=seam.phase==SeamTransit::Rollback?(seam.toPhone?-1:1):seam.toPhone?1:-1;}
  else{auto&m=swimmers[seam.fish];m.x=x;m.y=y;m.vy=0;m.direction=seam.phase==SeamTransit::Rollback?(seam.toPhone?-1:1):seam.toPhone?1:-1;}
}
bool nativeArrivalClear(){
  int i=seam.fish;float x=W-SeamTransit::width(i)+(i==2?18:i==3?1:0),y=seam.height*H;
  if(i<3)return canMove(i,x,y);
  for(int j=0;j<3;j++)if(fishVisible(j)){
    float l=swimmers[j].x-(j==2?18:0),r=l+SeamTransit::width(j),top=swimmers[j].y-(j==2?17:10),bottom=swimmers[j].y+(j==0?27:j==1?19:17);
    if(x-1<r+6&&x+37+6>l&&y<bottom+6&&y+21>top)return false;
  }
  return true;
}
void updateSeam(uint32_t now){
  if(!seam.active())return;
  if(seam.phase!=SeamTransit::Rollback&&!portal.allows(seam.client,seam.epoch,now))seam.cancel(now);
  if(seam.phase==SeamTransit::Offer&&seam.lead>0&&seam.raw(now)>=-8.f/SeamTransit::width(seam.fish))seam.cancel(now);
  if(seam.phase==SeamTransit::Offer&&!seam.toPhone&&nativeArrivalClear())seam.run(now);
  placeSeam(now);
  if(seam.phase==SeamTransit::Running&&seam.progress(now)>=1){
    auto result=commitTransfer(seam.fish,seam.client,seam.operation,seam.toPhone,seam.height,true);
    if(result==TransferResult::Changed||result==TransferResult::Unchanged){seam.phase=SeamTransit::Done;seam.changed=now;Serial.printf("PHONE_SEAM_DONE fish=%s toPhone=%d\n",creatureIds[seam.fish],seam.toPhone);}
    else seam.cancel(now);
  }else if(seam.phase==SeamTransit::Rollback&&seam.progress(now)<=seam.origin()+.00001f){
    if(seam.toPhone){int i=seam.fish;if(i==3){crabX=W-37-seam.lead;crabDirection=-1;}else{swimmers[i].x=W-SeamTransit::width(i)-seam.lead+(i==2?18:0);swimmers[i].direction=-1;swimmers[i].next=0;}}
    seam.phase=SeamTransit::Cancelled;seam.changed=now;
  }
}
bool nativeSourceClear(int i,float x,float y){
  float left=x-(i==2?18:i==3?1:0),top=y-(i==2?17:i==3?0:10),bottom=y+(i==0?27:i==1?19:i==2?17:15);
  for(int j=0;j<4;j++)if(j!=i&&fishVisible(j)){
    float ox=j==3?crabX:swimmers[j].x,oy=j==3?crabY:swimmers[j].y;
    float l=ox-(j==2?18:j==3?1:0),r=l+SeamTransit::width(j),t=oy-(j==2?17:j==3?0:10),b=oy+(j==0?27:j==1?19:j==2?17:15);
    if(left<r+6&&W+6>l&&top<b+6&&bottom+6>t)return false;
  }return true;
}
bool finishAtEdge(int index,float x){
  int direction=index==3?crabDirection:swimmers[index].direction;
  float right=x+(index==0?84:index==1?42:index==2?18:37);
  if(!fishInTank(index)||direction<0||!portal.live(millis())||right<W-48)return false;
  if(seam.active()){
    if(index==3){crabDirection=-1;nextCrabChoice=millis()+3000;}
    else{swimmers[index].direction=-1;swimmers[index].next=millis()+3000;}
    return true;
  }
  String op="seam-"+String(millis())+"-"+String(index)+"-"+String(ownership.revision);
  float y=index==3?crabY/H:swimmers[index].y/H,speed=index==3?crabSpeed:swimmers[index].vx;
  if(!nativeSourceClear(index,x,y*H))return false;
  seam.offer(index,true,portal.client,op.c_str(),portal.epoch,y,speed,millis(),fmaxf(0,W-right));placeSeam(millis());
  Serial.printf("PHONE_SEAM_OFFER fish=%s\n",creatureIds[index]);return true;
}
void beginHotspot() {
  networkWanted = false;
  if (networkReady) return;
  if (preferences.putBool("radioTrial", true) != 1) {
    Serial.println("PHONE_RADIO_SKIPPED fallback_marker_write_failed=1");
    return;
  }
  preferences.putBool("wifiEnabled", true);
  radioTrial = true;
  Serial.println("PHONE_RADIO_START brownout_protection=enabled");
  WiFi.persistent(false);
  bool ready = WiFi.mode(WIFI_AP);
  if (ready) {
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    ready = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, 2);
  }
  if (!ready) {
    preferences.putBool("wifiEnabled", false);
    preferences.putBool("radioTrial", false);
    radioTrial = false;
    WiFi.mode(WIFI_OFF);
    Serial.println("PHONE_RADIO_FAILED aquarium_only=1");
    return;
  }
  server.begin();
  networkReady = true;
  radioStarted = millis();
  Serial.printf("PHONE_HOTSPOT_READY ssid=%s ip=%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}
void phoneSetup() {
  bootToken=esp_random();
  pinMode(0, INPUT_PULLUP);
  if (!preferences.begin("pocketreef", false)) {
    Serial.println("PHONE_STORAGE_UNAVAILABLE aquarium_only=1");
    return;
  }
  size_t stored = preferences.getBytesLength("fish");
  memset(&ownership, 0, sizeof(ownership)); ownership.magic = OWNERSHIP_MAGIC;
  if (stored == sizeof(Ownership)) {
    preferences.getBytes("fish", &ownership, sizeof(ownership));
    if (ownership.magic != OWNERSHIP_MAGIC) { memset(&ownership, 0, sizeof(ownership)); ownership.magic = OWNERSHIP_MAGIC; }
  } else if (stored == sizeof(LegacyOwnership)) {
    LegacyOwnership old = {}; preferences.getBytes("fish", &old, sizeof(old));
    if (old.magic == 0x52454631) {
      ownership.revision = old.revision;
      for (int i = 0; i < 2; ++i) {
        memcpy(ownership.owner[i], old.owner[i], 40); memcpy(ownership.lastOp[i], old.lastOp[i], 64);
        ownership.lastTarget[i] = old.lastTarget[i];
      }
      preferences.putBytes("fish", &ownership, sizeof(ownership));
      Serial.println("PHONE_OWNERSHIP_MIGRATED existing_fish_preserved=1");
    }
  }
  for (int i = 0; i < CREATURE_COUNT; ++i) {
    ownership.owner[i][39] = 0; ownership.lastOp[i][63] = 0;
    if (ownership.owner[i][0] && !validToken(ownership.owner[i], 40)) ownership.owner[i][0] = 0;
  }
  bool failedTrial = preferences.getBool("radioTrial", false);
  if (failedTrial) {
    preferences.putBool("wifiEnabled", false);
    preferences.putBool("radioTrial", false);
    Serial.println("PHONE_SAFE_FALLBACK aquarium_only=1 hold_BOOT_2_seconds_to_retry");
  }
  networkWanted = preferences.getBool("wifiEnabled", true);
  server.on("/", HTTP_GET, [] { server.sendHeader("Cache-Control", "no-cache"); server.send_P(200, "text/html; charset=utf-8", WEB_PAGE); });
  server.on("/api/state", HTTP_GET, [] {
    String client = server.arg("client");
    if (!validToken(client.c_str(), 40)) { sendState("", 400, "Invalid phone identifier"); return; }
    sendState(client);
  });
  server.on("/api/portal", HTTP_POST, [] {
    String client = server.arg("client");
    if (!validToken(client.c_str(), 40)) { sendState("", 400, "Invalid phone identifier"); return; }
    bool nearby = server.arg("near") == "1";
    bool wasOpen = portal.live(millis());
    if (!portal.update(client.c_str(), nearby, millis())) { sendState(client, 409, "Another phone is using the edge"); return; }
    if (wasOpen != portal.live(millis())) Serial.printf("PHONE_PORTAL open=%d epoch=%lu\n", portal.live(millis()), (unsigned long)portal.epoch);
    updateSeam(millis());sendState(client);
  });
  server.on("/api/seam",HTTP_POST,[]{
    String client=server.arg("client"),op=server.arg("op"),action=server.arg("action");int i=creatureIndex(server.arg("fish"));uint32_t epoch=uint32_t(server.arg("epoch").toInt());
    if(i<0||!validToken(client.c_str(),40)||!validToken(op.c_str(),64)){sendState(client,400,"Invalid crossing");return;}
    if(!portal.allows(client.c_str(),epoch,millis())){sendState(client,412,"Edge closed");return;}
    if(action=="accept"){
      if(seam.fish!=i||!seam.toPhone||client!=seam.client||op!=seam.operation||epoch!=seam.epoch){sendState(client,409,"Crossing changed");return;}
      seam.run(millis());sendState(client);return;
    }
    if(action!="begin"||String(ownership.owner[i])!=client){sendState(client,409,"Ownership changed");return;}
    if(seam.active()){
      if(seam.fish==i&&client==seam.client&&op==seam.operation){sendState(client);return;}
      sendState(client,423,"Another creature is crossing");return;
    }
    if(server.arg("edge")!="left"||!server.hasArg("left")||server.arg("left").toFloat()>0.26f){sendState(client,409,"Not at edge");return;}
    float y=server.arg("height").toFloat(),speed=server.arg("speed").toFloat();
    y=fmaxf(i==2?28.f/H:24.f/H,fminf(i==3?220.f/H:216.f/H,y));
    seam.offer(i,false,client.c_str(),op.c_str(),epoch,y,speed,millis(),fmaxf(0,server.arg("left").toFloat()*W));updateSeam(millis());sendState(client);
  });
  server.on("/api/transfer",HTTP_POST,[]{sendState(server.arg("client"),409,"Refresh the phone page for joined crossing");});
  server.on("/health", HTTP_GET, [] { server.send(200, "application/json", "{\"ok\":true,\"version\":\"REEF_LINK_STABLE_V30\"}"); });
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  Serial.printf("PHONE_STATE_RESTORED revision=%lu tank_large=%d tank_small=%d\n",
                (unsigned long)ownership.revision, fishInTank(0), fishInTank(1));
}
void phoneTick(uint32_t now) {
  portal.expire(now);
  if (networkWanted && now > 2500) beginHotspot();
  if (networkReady) {
    server.handleClient();
    if (radioTrial && millis() - radioStarted > 20000) {
      preferences.putBool("radioTrial", false);
      radioTrial = false;
      Serial.println("PHONE_RADIO_STABLE trial_complete=1");
    }
  }
  updateSeam(millis());
  if (digitalRead(0) == LOW) {
    if (!buttonSince) buttonSince = now;
    if (now - buttonSince > 2000 && !networkReady) { networkWanted = true; buttonSince = now; }
  } else buttonSince = 0;
  for (int limit = 0; limit < 120 && Serial.available(); ++limit) {
    char c = Serial.read();
    if (c == '\n') {
      serialCommand.trim();
      if (serialCommand == "WIFI_ON") networkWanted = true;
      else if (serialCommand == "STATE") Serial.printf("PHONE_STATE revision=%lu tank_large=%d tank_small=%d tank_octopus=%d tank_crab=%d wifi=%d stations=%u portal=%d http=%lu httpAge=%lu idleDrops=%lu rssiMiss=%lu rssiFallback=%lu\n",
        (unsigned long)ownership.revision, fishInTank(0), fishInTank(1), fishInTank(2), fishInTank(3), networkReady,
        networkReady ? WiFi.softAPgetStationNum() : 0,portal.live(millis()),(unsigned long)httpRequests,
        (unsigned long)(lastHttpAt?millis()-lastHttpAt:0),(unsigned long)server.idleDrops,(unsigned long)rssiMisses,(unsigned long)rssiSingleFallbacks);
      else if (serialCommand == "RSSI") {
        wifi_sta_list_t list = {}; if (esp_wifi_ap_get_sta_list(&list) == ESP_OK)
          for (int i = 0; i < list.num; ++i) Serial.printf("PHONE_SIGNAL station=%d rssi=%d\n", i, list.sta[i].rssi);
      }
      else if (serialCommand == "REBOOT") ESP.restart();
      serialCommand = "";
    } else if (c != '\r' && serialCommand.length() < 150) serialCommand += c;
  }
}
