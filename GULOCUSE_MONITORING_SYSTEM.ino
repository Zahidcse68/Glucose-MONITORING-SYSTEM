#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "HX711.h"
#include <EEPROM.h>
#include <cmath>

// =====================================================
// WIFI
// =====================================================
const char* ssid = "iphone";
const char* password = "12345678";

// =====================================================
// HX711 PINS - YOUR EXISTING CONNECTIONS
// =====================================================
// NodeMCU ESP8266: HX711 DT=D6 (GPIO12), SCK=D5 (GPIO14)
// Buzzer=D1 (GPIO5)
#define HX711_DT  D6
#define HX711_SCK D5

HX711 scale;
ESP8266WebServer server(80);

// =====================================================
// EXISTING LOAD-CELL CALIBRATION
// =====================================================
long currentOffset = -171616;
float currentScale = 217.730606;
float maxWeight = 5000.0;

// =====================================================
// LIVE SENSOR DATA
// =====================================================
long rawReading = 0;
long lastRaw = 0;
float weight = 0;
float lastWeight = 0;
bool hx711Connected = false;
unsigned long lastGoodRead = 0;

// =====================================================
// BOTTLE CALIBRATION
// Empty/full values are total scale readings in grams.
// =====================================================
float emptyBottleWeight = 0.0;
float fullBottleWeight = 0.0;
float fullLiquidWeight = 0.0;
bool bottleCalibrated = false;

float liquidWeight = 0.0;
float liquidLevel = 0.0;
String bottleStatus = "NOT CALIBRATED";

// =====================================================
// BUZZER
// =====================================================
#define BUZZER_PIN D7 // 
#define BUZZER_THRESHOLD 40.0

unsigned long lastBeep = 0;
bool buzzerState = false;

// =====================================================
// MOBILE DASHBOARD - CLEAN TEXT UI (NO EMOJIS)
// =====================================================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Glucose Bottle Monitor</title>
<style>
*{box-sizing:border-box}
body{margin:0;padding:0;font-family:Arial,sans-serif;background:#07111f;color:white}
.container{max-width:700px;margin:auto;padding:15px}
.header{text-align:center;margin-bottom:15px;padding-top:10px}
.header h1{margin:0;color:#38bdf8;font-size:22px}
.header p{color:#8da2bb;font-size:13px;margin:5px 0}
.status-badge{display:inline-block;padding:5px 12px;border-radius:12px;font-size:12px;font-weight:bold;margin-top:5px}

/* MAIN SCREEN BOTTLE UI */
#mainScreen { text-align: center; }
.bottle-wrapper { margin: 30px auto 20px auto; width: 140px; }
.bottle-neck { width: 40px; height: 25px; border: 4px solid #40536d; border-bottom: none; border-radius: 8px 8px 0 0; margin: 0 auto; background: rgba(255,255,255,0.05); }
.bottle-body { width: 140px; height: 260px; border: 4px solid #40536d; border-radius: 20px 20px 15px 15px; position: relative; overflow: hidden; background: #0a1627; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
.liquid-fill { position: absolute; bottom: 0; left: 0; width: 100%; height: 0%; background: linear-gradient(180deg, #38bdf8, #0284c7); transition: height 0.8s ease, background 0.5s; }
.liquid-top { position: absolute; top: -5px; width: 100%; height: 10px; background: rgba(255,255,255,0.2); border-radius: 50%; }
.bottle-marks { position: absolute; top: 0; left: 10px; height: 100%; width: 20px; display: flex; flex-direction: column; justify-content: space-between; padding: 20px 0; z-index: 10; }
.mark { width: 15px; height: 2px; background: rgba(255,255,255,0.3); }
.mark.major { width: 25px; height: 3px; }

.main-reading { font-size: 48px; font-weight: bold; color: #fff; margin-top: 10px; }
.main-unit { font-size: 20px; color: #9cafc4; }
.main-percent { font-size: 18px; color: #38bdf8; margin-bottom: 30px; font-weight:bold; }

/* BUTTONS */
button { width:100%; padding:15px; border:0; border-radius:12px; color:white; font-size:15px; font-weight:bold; cursor:pointer; margin-top:10px; transition: opacity 0.2s; }
button:active { opacity: 0.7; }
.btn-settings { background: #1e293b; border: 1px solid #334155; color: #94a3b8; }
.btn-close { background: #334155; margin-bottom:20px; color: #fff; border: 1px solid #475569; }
.calibrateBottle { background:#2563eb }
.emptyButton { background:#16a34a }
.fullButton { background:#7c3aed }
.resetButton { background:#dc2626 }
.tare { background:#0891b2 }
.calibrate { background:#9333ea }

/* SETTINGS SCREEN CARDS */
#settingsScreen { display: none; }
.card{background:#111e31;border:1px solid #293c56;border-radius:16px;padding:20px;margin-bottom:16px}
.title{color:#8da2bb;font-size:14px;margin-bottom:10px;font-weight:bold; text-transform: uppercase;}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.stat{background:#0a1627;padding:14px;border-radius:12px;min-width:0}
.statTitle{font-size:11px;color:#7f94ad}
.statValue{margin-top:6px;font-size:18px;font-weight:bold;word-break:break-word}

.ready,.normal{color:#4ade80}
.warning{color:#facc15}
.danger,.error{color:#f87171}
.status-ready { background: rgba(74, 222, 128, 0.15); color: #4ade80; }
.status-error { background: rgba(248, 113, 113, 0.15); color: #f87171; }
.status-warning { background: rgba(250, 204, 21, 0.15); color: #facc15; }

input{width:100%;padding:13px;margin-top:9px;border-radius:9px;border:1px solid #40536d;background:#07111f;color:white;font-size:16px}
.message{text-align:center;margin-top:12px;color:#4ade80;line-height:1.5; font-size:14px;}
.calibrationBox{background:#0a1627;padding:15px;border-radius:13px;margin-top:10px}
.small{color:#8da2bb;font-size:13px;line-height:1.5}
.footer{text-align:center;color:#475569;font-size:12px;margin-top:15px; margin-bottom:30px;}
</style>
</head>
<body>
<div class="container">

  <div class="header">
    <h1>GLUCOSE BOTTLE</h1>
    <div id="status" class="status-badge status-ready">STATUS: LIVE</div>
  </div>

  <!-- MAIN BOTTLE SCREEN -->
  <div id="mainScreen">
    <div class="bottle-wrapper">
      <div class="bottle-neck"></div>
      <div class="bottle-body">
        <div class="bottle-marks">
          <div class="mark major"></div><div class="mark"></div>
          <div class="mark major"></div><div class="mark"></div>
          <div class="mark major"></div><div class="mark"></div>
          <div class="mark major"></div>
        </div>
        <div id="liquidFill" class="liquid-fill">
          <div class="liquid-top"></div>
        </div>
      </div>
    </div>

    <div class="main-reading">
      <span id="liquidWeightMain">0.0</span><span class="main-unit"> g</span>
    </div>
    <div class="main-percent">
      <span id="liquidLevelMain">0</span>% REMAINING
    </div>

    <button class="btn-settings" onclick="openSettings()">CALIBRATION AND SETTINGS</button>
  </div>


  <!-- SETTINGS & CALIBRATION SCREEN (HIDDEN BY DEFAULT) -->
  <div id="settingsScreen">
    <button class="btn-close" onclick="closeSettings()"><< BACK TO MONITOR</button>

    <!-- BOTTLE CALIBRATION -->
    <div class="card">
      <div class="title">BOTTLE CALIBRATION</div>
      <p class="small">First place EMPTY bottle and set. Then place FULL bottle and set. Keep bottle still.</p>
      
      <div class="calibrationBox">
        <div class="statTitle">EMPTY BOTTLE WEIGHT</div>
        <div id="emptyBottle" class="statValue">0.00 g</div>
        <button class="emptyButton" onclick="calibrateEmpty()">SET EMPTY BOTTLE</button>
      </div>

      <div class="calibrationBox">
        <div class="statTitle">FULL BOTTLE WEIGHT</div>
        <div id="fullBottle" class="statValue">0.00 g</div>
        <button class="fullButton" onclick="calibrateFull()">SET FULL BOTTLE</button>
      </div>

      <button class="resetButton" onclick="resetBottleCalibration()">RESET BOTTLE CALIBRATION</button>
      <div id="bottleMessage" class="message"></div>
    </div>

    <!-- TARE -->
    <div class="card">
      <div class="title">ZERO / TARE SCALE</div>
      <p class="small">Remove all weight from the load cell before pressing TARE.</p>
      <button class="tare" onclick="tareScale()">TARE SCALE</button>
    </div>

    <!-- LOAD CELL CALIBRATION -->
    <div class="card">
      <div class="title">LOAD CELL CALIBRATION</div>
      <p class="small">Calibrate the HX711 scale factor using a known standard weight.</p>
      <input id="knownWeight" type="number" placeholder="Known standard weight in grams" step="0.1" min="1">
      <button class="calibrate" onclick="calibrateScale()">CALIBRATE LOAD CELL</button>
      <div id="message" class="message"></div>
    </div>

    <!-- DIAGNOSTIC INFORMATION -->
    <div class="card">
      <div class="title">SYSTEM DIAGNOSTICS</div>
      <div class="grid">
        <div class="stat"><div class="statTitle">TOTAL WEIGHT</div><div id="totalWeight" class="statValue">0.00 g</div></div>
        <div class="stat"><div class="statTitle">LIQUID REMAINING</div><div id="liquid" class="statValue">0.00 g</div></div>
        <div class="stat"><div class="statTitle">FULL CAPACITY</div><div id="fullLiquid" class="statValue">0.00 g</div></div>
        <div class="stat"><div class="statTitle">BOTTLE STATUS</div><div id="bottleStatus" class="statValue">NOT CALIBRATED</div></div>
        <div class="stat"><div class="statTitle">RAW READING</div><div id="raw" class="statValue">0</div></div>
        <div class="stat"><div class="statTitle">SCALE FACTOR</div><div id="scale" class="statValue">0</div></div>
      </div>
    </div>

  </div>

  <div class="footer">ESP8266 + HX711 SMART MONITOR</div>
</div>

<script>
function openSettings() {
  document.getElementById('mainScreen').style.display = 'none';
  document.getElementById('settingsScreen').style.display = 'block';
}

function closeSettings() {
  document.getElementById('settingsScreen').style.display = 'none';
  document.getElementById('mainScreen').style.display = 'block';
}

async function updateData(){
  try{
    const response=await fetch("/data");
    const d=await response.json();

    const total=Math.max(0,Number(d.weight)||0);
    const liquid=Math.max(0,Number(d.liquid)||0);
    const level=Math.min(100,Math.max(0,Number(d.level)||0));

    // Update Main Screen
    document.getElementById("liquidWeightMain").innerText=liquid.toFixed(1);
    document.getElementById("liquidLevelMain").innerText=level.toFixed(0);
    
    // Update Bottle Graphic Height & Color
    const liquidFill = document.getElementById("liquidFill");
    liquidFill.style.height = level + "%";
    
    if(d.bottleStatus === "LOW" || d.bottleStatus === "EMPTY") {
       liquidFill.style.background = "linear-gradient(180deg, #ef4444, #991b1b)"; // Red warning
       document.getElementById("liquidLevelMain").style.color = "#ef4444";
    } else {
       liquidFill.style.background = "linear-gradient(180deg, #38bdf8, #0284c7)"; // Normal blue
       document.getElementById("liquidLevelMain").style.color = "#38bdf8";
    }

    // Update Settings Screen Data
    document.getElementById("totalWeight").innerText=total.toFixed(2)+" g";
    document.getElementById("liquid").innerText=liquid.toFixed(2)+" g";
    
    const bs=document.getElementById("bottleStatus");
    bs.innerText=d.bottleStatus;
    bs.className="statValue";
    if(d.bottleStatus==="LOW") bs.classList.add("warning");
    else if(d.bottleStatus==="EMPTY") bs.classList.add("danger");
    else if(d.bottleStatus==="NORMAL") bs.classList.add("normal");
    else bs.classList.add("error");

    const empty=Number(d.emptyBottle)||0;
    const full=Number(d.fullBottle)||0;
    const capacity=Number(d.fullLiquid)||0;

    document.getElementById("emptyBottle").innerText=empty.toFixed(2)+" g";
    document.getElementById("fullBottle").innerText=full.toFixed(2)+" g";
    document.getElementById("fullLiquid").innerText=capacity.toFixed(2)+" g";

    document.getElementById("raw").innerText=d.raw;
    document.getElementById("scale").innerText=Number(d.scale).toFixed(2);

    const statusBadge = document.getElementById("status");
    if(d.ready){
      if(!d.bottleCalibrated) {
        statusBadge.innerText="STATUS: SETUP NEEDED";
        statusBadge.className="status-badge status-warning";
      } else if (d.bottleStatus==="LOW" || d.bottleStatus==="EMPTY") {
        statusBadge.innerText="STATUS: " + d.bottleStatus;
        statusBadge.className="status-badge status-error";
      } else {
        statusBadge.innerText="STATUS: NORMAL";
        statusBadge.className="status-badge status-ready";
      }
    }else{
      statusBadge.innerText="STATUS: SENSOR ERROR";
      statusBadge.className="status-badge status-error";
    }
  }catch(e){
    document.getElementById("status").innerText="STATUS: OFFLINE";
    document.getElementById("status").className="status-badge status-error";
  }
}

async function postAction(url,messageId){
  const msg=document.getElementById(messageId);
  msg.innerText="Processing...";
  try{
    const response=await fetch(url,{method:"POST"});
    const data=await response.json();
    msg.innerText=data.message;
    await updateData();
  }catch(e){
    msg.innerText="Request failed.";
  }
}

function calibrateEmpty(){ postAction("/bottle-empty","bottleMessage"); }
function calibrateFull(){ postAction("/bottle-full","bottleMessage"); }
async function resetBottleCalibration(){
  if(!confirm("Reset bottle calibration?")) return;
  postAction("/bottle-reset","bottleMessage");
}
function tareScale(){ postAction("/tare","message"); }
async function calibrateScale(){
  const knownWeight=Number(document.getElementById("knownWeight").value);
  if(!isFinite(knownWeight)||knownWeight<=0){
    alert("Enter a valid known weight in grams.");
    return;
  }
  const msg=document.getElementById("message");
  msg.innerText="Calibrating...";
  try{
    const response=await fetch("/calibrate?weight="+encodeURIComponent(knownWeight),{method:"POST"});
    const data=await response.json();
    msg.innerText=data.message;
    await updateData();
  }catch(e){
    msg.innerText="Calibration failed.";
  }
}

setInterval(updateData,500);
updateData();
</script>
</body>
</html>
)rawliteral";

// =====================================================
// CALCULATE LIQUID WEIGHT AND LEVEL
// =====================================================
void calculateBottleValues() {
  if (!bottleCalibrated || fullLiquidWeight <= 0) {
    liquidWeight = 0;
    liquidLevel = 0;
    bottleStatus = "NOT CALIBRATED";
    return;
  }

  liquidWeight = lastWeight - emptyBottleWeight;
  if (liquidWeight < 0) liquidWeight = 0;

  liquidLevel = (liquidWeight / fullLiquidWeight) * 100.0;
  if (liquidLevel < 0) liquidLevel = 0;
  if (liquidLevel > 100) liquidLevel = 100;

  if (liquidWeight <= 0) {
    bottleStatus = "EMPTY";
  } else if (liquidWeight < BUZZER_THRESHOLD) {
    bottleStatus = "LOW";
  } else {
    bottleStatus = "NORMAL";
  }
}

// =====================================================
// EEPROM STORAGE FOR ESP8266
// =====================================================
struct BottleCalibrationData {
  uint32_t magic;
  float emptyWeight;
  float fullWeight;
  float liquidCapacity;
  bool calibrated;
};

const uint32_t EEPROM_MAGIC = 0x474C5543;

void saveBottleCalibration() {
  BottleCalibrationData data;
  data.magic = EEPROM_MAGIC;
  data.emptyWeight = emptyBottleWeight;
  data.fullWeight = fullBottleWeight;
  data.liquidCapacity = fullLiquidWeight;
  data.calibrated = bottleCalibrated;

  EEPROM.begin(sizeof(BottleCalibrationData));
  EEPROM.put(0, data);
  EEPROM.commit();
  EEPROM.end();
}

void loadBottleCalibration() {
  BottleCalibrationData data;
  EEPROM.begin(sizeof(BottleCalibrationData));
  EEPROM.get(0, data);
  EEPROM.end();

  if (data.magic == EEPROM_MAGIC &&
      data.liquidCapacity > 0 &&
      data.fullWeight > data.emptyWeight) {
    emptyBottleWeight = data.emptyWeight;
    fullBottleWeight = data.fullWeight;
    fullLiquidWeight = data.liquidCapacity;
    bottleCalibrated = data.calibrated;
  } else {
    emptyBottleWeight = 0;
    fullBottleWeight = 0;
    fullLiquidWeight = 0;
    bottleCalibrated = false;
  }
}

// =====================================================
// READ HX711
// =====================================================
void readHX711() {
  if (scale.is_ready()) {
    long raw = scale.read();

    if (raw != 0) {
      rawReading = raw;
      float newWeight = scale.get_units(3);

      if (std::isfinite(newWeight)) {
        weight = newWeight;
        lastRaw = rawReading;
        lastWeight = weight;
        hx711Connected = true;
        lastGoodRead = millis();
      }
    }
  }

  if (millis() - lastGoodRead > 3000) {
    hx711Connected = false;
  }

  calculateBottleValues();
}

// =====================================================
// JSON DATA API
// =====================================================
void handleData() {
  readHX711();

  String json = "{";
  json += "\"weight\":" + String(lastWeight, 2);
  json += ",\"raw\":" + String(lastRaw);
  json += ",\"offset\":" + String(currentOffset);
  json += ",\"scale\":" + String(currentScale, 6);
  json += ",\"max\":" + String(maxWeight, 0);
  json += ",\"ready\":";
  json += (hx711Connected ? "true" : "false");
  json += ",\"liquid\":" + String(liquidWeight, 2);
  json += ",\"level\":" + String(liquidLevel, 2);
  json += ",\"bottleStatus\":\"" + bottleStatus + "\"";
  json += ",\"emptyBottle\":" + String(emptyBottleWeight, 2);
  json += ",\"fullBottle\":" + String(fullBottleWeight, 2);
  json += ",\"fullLiquid\":" + String(fullLiquidWeight, 2);
  json += ",\"bottleCalibrated\":";
  json += (bottleCalibrated ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// =====================================================
// SCALE TARE
// IMPORTANT: remove everything from the load cell first
// =====================================================
void handleTare() {
  Serial.println("\nTARING...");

  scale.set_scale(currentScale);
  scale.tare(20);
  currentOffset = scale.get_offset();
  lastWeight = 0;

  String json = "{\"message\":\"TARE completed. New offset: ";
  json += String(currentOffset);
  json += "\"}";

  server.send(200, "application/json", json);
}

// =====================================================
// LOAD-CELL CALIBRATION USING KNOWN STANDARD WEIGHT
// =====================================================
void handleCalibration() {
  if (!scale.is_ready()) {
    server.send(503, "application/json",
                "{\"message\":\"HX711 is not ready. Check wiring.\"}");
    return;
  }

  float knownWeight = server.arg("weight").toFloat();

  if (knownWeight <= 0 || !std::isfinite(knownWeight)) {
    server.send(400, "application/json",
                "{\"message\":\"Invalid weight.\"}");
    return;
  }

  Serial.println("\nLOAD CELL CALIBRATION");
  Serial.print("Known weight: ");
  Serial.print(knownWeight);
  Serial.println(" g");

  long raw = scale.read_average(20);
  long offset = scale.get_offset();
  float newScale = (float)(raw - offset) / knownWeight;

  if (!std::isfinite(newScale) || newScale == 0) {
    server.send(400, "application/json",
                "{\"message\":\"Calibration calculation failed.\"}");
    return;
  }

  currentOffset = offset;
  currentScale = newScale;
  scale.set_offset(currentOffset);
  scale.set_scale(currentScale);

  Serial.print("NEW SCALE: ");
  Serial.println(currentScale, 6);

  String json = "{\"message\":\"Load cell calibration complete. New scale: ";
  json += String(currentScale, 6);
  json += "\"}";

  server.send(200, "application/json", json);
}

// =====================================================
// HELPER: TAKE A STABLE WEIGHT READING
// =====================================================
bool getStableWeight(float &measuredWeight) {
  if (!scale.wait_ready_timeout(1500)) {
    return false;
  }

  measuredWeight = scale.get_units(20);

  if (!std::isfinite(measuredWeight)) {
    return false;
  }

  return true;
}

// =====================================================
// CAPTURE EMPTY BOTTLE WEIGHT
// =====================================================
void handleBottleEmpty() {
  float measuredWeight = 0;

  if (!getStableWeight(measuredWeight)) {
    server.send(503, "application/json",
                "{\"message\":\"HX711 not ready. Check sensor and try again.\"}");
    return;
  }

  if (measuredWeight < 0) measuredWeight = 0;

  emptyBottleWeight = measuredWeight;

  // Recalculate capacity if a full bottle value was already stored.
  if (fullBottleWeight > emptyBottleWeight) {
    fullLiquidWeight = fullBottleWeight - emptyBottleWeight;
    bottleCalibrated = (fullLiquidWeight > 0);
  } else {
    fullLiquidWeight = 0;
    bottleCalibrated = false;
  }

  saveBottleCalibration();
  calculateBottleValues();

  Serial.print("EMPTY BOTTLE: ");
  Serial.print(emptyBottleWeight, 2);
  Serial.println(" g");

  String json = "{\"message\":\"Empty bottle saved: ";
  json += String(emptyBottleWeight, 2);
  json += " g\"}";

  server.send(200, "application/json", json);
}

// =====================================================
// CAPTURE FULL BOTTLE WEIGHT
// =====================================================
void handleBottleFull() {
  if (emptyBottleWeight <= 0) {
    server.send(400, "application/json",
                "{\"message\":\"First calibrate the EMPTY bottle.\"}");
    return;
  }

  float measuredWeight = 0;

  if (!getStableWeight(measuredWeight)) {
    server.send(503, "application/json",
                "{\"message\":\"HX711 not ready. Check sensor and try again.\"}");
    return;
  }

  if (measuredWeight <= emptyBottleWeight) {
    server.send(400, "application/json",
                "{\"message\":\"Full bottle weight must be greater than empty bottle weight.\"}");
    return;
  }

  fullBottleWeight = measuredWeight;
  fullLiquidWeight = fullBottleWeight - emptyBottleWeight;

  if (fullLiquidWeight <= 0) {
    bottleCalibrated = false;
    server.send(400, "application/json",
                "{\"message\":\"Invalid liquid capacity.\"}");
    return;
  }

  bottleCalibrated = true;
  saveBottleCalibration();
  calculateBottleValues();

  Serial.print("FULL BOTTLE: ");
  Serial.print(fullBottleWeight, 2);
  Serial.println(" g");

  Serial.print("FULL LIQUID CAPACITY: ");
  Serial.print(fullLiquidWeight, 2);
  Serial.println(" g");

  String json = "{\"message\":\"Full bottle saved: ";
  json += String(fullBottleWeight, 2);
  json += " g. Liquid capacity: ";
  json += String(fullLiquidWeight, 2);
  json += " g.\"}";

  server.send(200, "application/json", json);
}

// =====================================================
// RESET BOTTLE CALIBRATION
// =====================================================
void handleBottleReset() {
  emptyBottleWeight = 0;
  fullBottleWeight = 0;
  fullLiquidWeight = 0;
  bottleCalibrated = false;
  liquidWeight = 0;
  liquidLevel = 0;
  bottleStatus = "NOT CALIBRATED";

  BottleCalibrationData data = {};
  data.magic = EEPROM_MAGIC;
  data.emptyWeight = 0;
  data.fullWeight = 0;
  data.liquidCapacity = 0;
  data.calibrated = false;

  EEPROM.begin(sizeof(BottleCalibrationData));
  EEPROM.put(0, data);
  EEPROM.commit();
  EEPROM.end();

  server.send(200, "application/json",
              "{\"message\":\"Bottle calibration reset.\"}");
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP8266 GLUCOSE BOTTLE MONITOR");
  Serial.println("================================");

  // HX711
  scale.begin(HX711_DT, HX711_SCK);
  scale.set_offset(currentOffset);
  scale.set_scale(currentScale);

  Serial.print("OFFSET: ");
  Serial.println(currentOffset);
  Serial.print("SCALE: ");
  Serial.println(currentScale, 6);

  Serial.println("Checking HX711...");
  if (scale.wait_ready_timeout(3000)) {
    Serial.println("HX711 FOUND!");
  } else {
    Serial.println("HX711 NOT READY!");
  }

  // Load saved bottle calibration
  loadBottleCalibration();

  Serial.print("Saved bottle calibration: ");
  Serial.println(bottleCalibrated ? "YES" : "NO");

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  unsigned long wifiStart = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - wifiStart < 20000) {
    delay(500);
    yield();
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("Dashboard: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timed out. Check credentials and restart.");
  }

  // Web server routes
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/data", HTTP_GET, handleData);
  server.on("/tare", HTTP_POST, handleTare);
  server.on("/calibrate", HTTP_POST, handleCalibration);

  server.on("/bottle-empty", HTTP_POST, handleBottleEmpty);
  server.on("/bottle-full", HTTP_POST, handleBottleFull);
  server.on("/bottle-reset", HTTP_POST, handleBottleReset);

  server.begin();
  Serial.println("Web server started!");
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {
  server.handleClient();
  yield();

  // Update sensor data continuously
  readHX711();

  // Beep only after bottle calibration, based on liquid
  // remaining (not total bottle weight).
  if (bottleCalibrated &&
      hx711Connected &&
      liquidWeight < BUZZER_THRESHOLD) {

    if (millis() - lastBeep >= 500) {
      lastBeep = millis();
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
    }

  } else {
    buzzerState = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}
