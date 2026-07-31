#include <WiFi.h>
#include <WebServer.h>

// ==========================================================
// Stage 3 - EVA Memory Relay Web Challenge
// ESP32 Access Point + Embedded Web Game (HTML/CSS/JS inline)
// ==========================================================

// ---------- WiFi AP Configuration ----------
const char* AP_SSID = "EVA-MEMORY-3";
const char* AP_PASSWORD = "1234567890";  // Shared six-digit access code

// ---------- Next Stage Output ----------
const char* NEXT_ROOM = "Room No: 4";
const char* NEXT_CODE = "123456";

WebServer server(80);

// Embedded HTML (no SPIFFS, no external files)
String buildPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Stage 3 • Memory Relay</title>
  <style>
    :root{
      --bg:#f6f1e7;
      --paper:#fffaf1;
      --ink:#1f1f1f;
      --muted:#666;
      --accent:#7a4c1d;
      --ok:#1f7a3d;
      --danger:#a32020;
      --card:#fff;
      --shadow:0 12px 28px rgba(0,0,0,.12);
    }
    *{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
    body{
      margin:0;min-height:100vh;
      font-family:Segoe UI,Inter,Arial,sans-serif;color:var(--ink);
      background:
        radial-gradient(circle at 10% 20%, rgba(183,123,59,.08), transparent 30%),
        radial-gradient(circle at 90% 80%, rgba(122,76,29,.08), transparent 30%),
        var(--bg);
      display:grid;place-items:center;padding:18px;
    }
    .wrap{
      width:min(980px,96vw);background:var(--paper);border:2px solid #d7cbb8;
      border-radius:18px;box-shadow:var(--shadow);padding:24px;
    }
    .head{display:flex;justify-content:space-between;align-items:center;gap:8px;
      border-bottom:1px dashed #ccbba4;padding-bottom:10px;margin-bottom:14px}
    .title{font-weight:800;letter-spacing:.04em;text-transform:uppercase}
    .status{font-size:.88rem;color:var(--muted)}
    h1{margin:.2rem 0 .4rem;font-size:clamp(1.3rem,2.8vw,2.2rem)}
    .sub{margin:0 0 14px;color:var(--muted)}
    .grid{
      display:grid;grid-template-columns:1fr 1fr;gap:16px;align-items:start;
    }
    .panel{
      background:var(--card);border:1.5px solid #dacbb5;border-radius:14px;padding:14px;
    }
    .pads{
      display:grid;grid-template-columns:repeat(2,1fr);gap:12px;margin-top:8px;
    }
    .pad{
      border:none;border-radius:14px;min-height:100px;cursor:pointer;
      font-size:1.05rem;font-weight:700;color:#fff;opacity:.9;transition:.15s;
      box-shadow:0 6px 14px rgba(0,0,0,.14);
    }
    .pad:active{transform:scale(.98)}
    .pad.locked{pointer-events:none;opacity:.55}
    .p0{background:#2f80ed}
    .p1{background:#eb5757}
    .p2{background:#27ae60}
    .p3{background:#f2994a}
    .pad.flash{filter:brightness(1.28);transform:scale(1.02)}
    .controls{display:flex;gap:10px;flex-wrap:wrap;margin-top:12px}
    button{
      border:1px solid #cdb99e;background:#fff;padding:10px 14px;border-radius:10px;
      font-weight:700;cursor:pointer;
    }
    button.primary{background:#f3e4cf;border-color:#b79163;color:#4d2f0f}
    .msg{min-height:1.4em;margin-top:8px;color:var(--muted);font-weight:600}
    .msg.ok{color:var(--ok)} .msg.err{color:var(--danger)}
    .meta{display:grid;gap:8px;font-size:.96rem}
    .codeBox{
      margin-top:10px;padding:12px;border:2px dashed #c7b092;border-radius:12px;
      background:#fff7ea;display:none;
    }
    .codeBox.show{display:block}
    .code{
      font-size:1.3rem;font-weight:800;letter-spacing:.08em;color:#4d2f0f;
    }
    .footer{border-top:1px dashed #ccbba4;margin-top:14px;padding-top:10px;color:#6b6257;font-size:.9rem}
    @media (max-width:780px){.grid{grid-template-columns:1fr}.pad{min-height:86px}}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="head">
      <div class="title">EVA ARCHIVE // ROOM 3</div>
      <div class="status">COMPANION-07 MEMORY RELAY</div>
    </div>

    <h1>A familiar process wakes up.</h1>
    <p class="sub">"I knew EVA before the world stopped listening. Help me replay this memory, and I may remember what she hid."</p>
    <p class="sub"><b>COMPANION-07:</b> I kept one harmless memory of EVA. Rebuild the sequence, one signal at a time.</p>

    <div class="grid">
      <section class="panel">
        <strong>Memory Relay</strong>
        <div class="pads">
          <button class="pad p0 locked" data-id="0">ALPHA</button>
          <button class="pad p1 locked" data-id="1">BETA</button>
          <button class="pad p2 locked" data-id="2">GAMMA</button>
          <button class="pad p3 locked" data-id="3">DELTA</button>
        </div>
        <div class="controls">
          <button class="primary" id="startBtn">Restore Memory</button>
          <button id="resetBtn">Reset</button>
        </div>
        <div class="msg" id="msg">Press <b>Restore Memory</b> to begin.</div>
      </section>

      <section class="panel">
        <strong>Mission Status</strong>
        <div class="meta">
          <div>Current Round: <span id="round">0</span></div>
          <div>Your Input: <span id="progress">0</span></div>
          <div>Target to Win: <span id="target">3 rounds</span></div>
        </div>

        <div class="codeBox" id="resultBox">
          <div><b>Transmission Restored ✅</b></div>
          <div style="margin-top:8px;">COMPANION-07: "I remember. EVA trusted the people who could still communicate."</div>
          <div style="margin-top:8px;">Next Destination: <b>%NEXT_ROOM%</b></div>
          <div style="margin-top:6px;">Shared Access Code:</div>
          <div class="code">%NEXT_CODE%</div>
        </div>
      </section>
    </div>

    <div class="footer">Hint: Stay calm, watch pattern rhythm, and repeat in order.</div>
  </div>

<script>
(() => {
  "use strict";

  const pads = [...document.querySelectorAll(".pad")];
  const startBtn = document.getElementById("startBtn");
  const resetBtn = document.getElementById("resetBtn");
  const msg = document.getElementById("msg");
  const roundEl = document.getElementById("round");
  const progressEl = document.getElementById("progress");
  const resultBox = document.getElementById("resultBox");

  const WIN_ROUNDS = 3;
  const sequence = [];
  let playerIndex = 0;
  let round = 0;
  let acceptingInput = false;
  let locked = true;
  let gameRunning = false;

  function setMsg(text, type = "") {
    msg.className = "msg";
    if (type === "ok") msg.classList.add("ok");
    if (type === "err") msg.classList.add("err");
    msg.innerHTML = text;
  }

  function sleep(ms){ return new Promise(res => setTimeout(res, ms)); }

  function randomPad() { return Math.floor(Math.random() * 4); }

  function lockPads(state) {
    locked = state;
    pads.forEach(p => p.classList.toggle("locked", state));
  }

  function updateStats() {
    roundEl.textContent = String(round);
    progressEl.textContent = String(playerIndex);
  }

  async function flashPad(id, duration = 380) {
    const el = pads[id];
    el.classList.add("flash");
    await sleep(duration);
    el.classList.remove("flash");
  }

  async function playSequence() {
    acceptingInput = false;
    lockPads(true);
    setMsg("A fragmented memory is replaying... watch carefully.");
    await sleep(500);

    for (const id of sequence) {
      await flashPad(id, 350);
      await sleep(220);
    }

    playerIndex = 0;
    updateStats();
    acceptingInput = true;
    lockPads(false);
    setMsg("Your turn: repeat the sequence.");
  }

  async function nextRound() {
    round++;
    sequence.push(randomPad());
    updateStats();
    await playSequence();
  }

  async function handlePadClick(id) {
    if (!acceptingInput || locked) return;

    await flashPad(id, 150);

    if (id !== sequence[playerIndex]) {
      acceptingInput = false;
      lockPads(true);
      gameRunning = false;
      setMsg("Relay mismatch ❌ Wrong order. Press Start to retry.", "err");
      return;
    }

    playerIndex++;
    updateStats();

    if (playerIndex === sequence.length) {
      if (round >= WIN_ROUNDS) {
        acceptingInput = false;
        lockPads(true);
        gameRunning = false;
        resultBox.classList.add("show");
        resultBox.scrollIntoView({ behavior: "smooth", block: "center" });
        setMsg("Mission Complete ✅ Network restored.", "ok");
        return;
      }

      setMsg("Round cleared ✅ Preparing next signal...", "ok");
      await sleep(700);
      await nextRound();
    }
  }

  function resetGame() {
    sequence.length = 0;
    playerIndex = 0;
    round = 0;
    acceptingInput = false;
    gameRunning = false;
    resultBox.classList.remove("show");
    lockPads(true);
    updateStats();
    setMsg("Press <b>Restore Memory</b> to begin.");
  }

  startBtn.addEventListener("click", async () => {
    if (gameRunning) return;
    resetGame();
    gameRunning = true;
    setMsg("Initializing relay challenge...");
    await sleep(300);
    await nextRound();
  });

  resetBtn.addEventListener("click", resetGame);

  pads.forEach(btn => {
    btn.addEventListener("click", () => handlePadClick(Number(btn.dataset.id)));
  });

  resetGame();
})();
</script>
</body>
</html>
)rawliteral";

  html.replace("%NEXT_ROOM%", NEXT_ROOM);
  html.replace("%NEXT_CODE%", NEXT_CODE);
  return html;
}

void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found. Open the main challenge page at /");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== Stage 3: EVA MEMORY RELAY BOOTING ===");

  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (!apOk) {
    Serial.println("ERROR: Failed to start Access Point!");
    while (true) {
      delay(1000);
    }
  }

  IPAddress ip = WiFi.softAPIP();
  Serial.println("AP Started Successfully");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("Open: http://");
  Serial.println(ip);

  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
  delay(2);
}
