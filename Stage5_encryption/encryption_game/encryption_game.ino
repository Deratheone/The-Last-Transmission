#include <WiFi.h>
#include <WebServer.h>

// ===========================================================
// Stage 5 - EVA Encrypted Memory Recovery Terminal
// ESP32 AP + embedded story + decryption challenge
// No SPIFFS, no external files
// ===========================================================

// ---------- AP credentials ----------
const char* AP_SSID = "EVA-CACHE-5";
const char* AP_PASSWORD = "1234567890";   // Previous stage's shared access code

// ---------- Challenge answer ----------
const char* EXPECTED_ANSWER = "EVA REMEMBERS TIME";
// Case-insensitive; spaces are ignored by the validator.

// ---------- Next stage reveal ----------
const char* NEXT_ROOM = "Room No: 6";
const char* NEXT_CODE = "123456"; // Fragment 5 of the final master key.

WebServer server(80);

// Normalize string: uppercase + remove spaces/tabs/newlines
String normalizeInput(const String& in) {
  String out = "";
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
    out += (char)toupper(c);
  }
  return out;
}

void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
<title>Stage 5 • EVA Recovery Terminal</title>
<style>
  :root{
    --bg:#0a0d12;
    --panel:#11161f;
    --panel2:#171f2b;
    --ink:#d7e1f0;
    --muted:#8fa0ba;
    --accent:#5eead4;
    --accent2:#7dd3fc;
    --warn:#fbbf24;
    --good:#22c55e;
    --bad:#ef4444;
    --line:#263248;
    --mono: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
  }
  *{box-sizing:border-box}
  body{
    margin:0; min-height:100vh; color:var(--ink);
    font-family:Inter,Segoe UI,Arial,sans-serif;
    background:
      radial-gradient(circle at 12% 8%, #16233a 0%, transparent 40%),
      radial-gradient(circle at 88% 92%, #142035 0%, transparent 45%),
      var(--bg);
    display:grid; place-items:center; padding:20px;
    overflow-x:hidden;
  }

  .terminal{
    position:relative;
    width:min(980px,96vw);
    border:1px solid var(--line);
    border-radius:16px;
    background:linear-gradient(180deg,var(--panel),var(--panel2));
    box-shadow:0 25px 60px rgba(0,0,0,.55), 0 0 0 1px rgba(94,234,212,.04);
    overflow:hidden;
  }

  /* faint scanline overlay for that terminal feel */
  .terminal::after{
    content:"";
    position:absolute; inset:0; pointer-events:none;
    background:repeating-linear-gradient(
      to bottom,
      rgba(255,255,255,0.015) 0px,
      rgba(255,255,255,0.015) 1px,
      transparent 2px,
      transparent 3px
    );
    mix-blend-mode:overlay;
    z-index:5;
  }

  .topbar{
    display:flex; justify-content:space-between; align-items:center;
    padding:12px 16px; border-bottom:1px solid var(--line);
    background:#0d121a;
    font-size:.85rem; color:var(--muted);
  }
  .dot{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:6px}
  .red{background:#ff5f56}.yel{background:#ffbd2e}.grn{background:#27c93f}
  .pulse{
    width:8px;height:8px;border-radius:50%;background:var(--accent);
    display:inline-block; margin-right:6px;
    box-shadow:0 0 8px var(--accent);
    animation:blink 1.6s ease-in-out infinite;
  }
  @keyframes blink{0%,100%{opacity:1}50%{opacity:.25}}

  .body{padding:22px}

  h1{
    margin:0 0 4px; font-size:clamp(1.15rem,2.6vw,1.8rem);
    letter-spacing:.01em;
  }
  .robot{
    color:var(--accent); font-weight:800;
    text-shadow:0 0 14px rgba(94,234,212,.35);
  }
  .sub{color:var(--muted); font-size:.85rem; margin-bottom:16px}

  .story{
    border:1px solid var(--line); border-radius:12px; padding:16px;
    background:#0c121b; line-height:1.6; color:#c8d5ea;
    margin-bottom:16px;
  }
  .story p{margin:.5rem 0}

  .dataBox{
    margin-top:12px;
    border:1px dashed #33455f; border-radius:12px; background:#080d15;
    padding:14px;
    font-family:var(--mono); font-size:1.05rem; color:var(--accent2);
    position:relative;
    letter-spacing:.03em;
  }
  .dataBox .tag{
    color:#5a7191; font-size:.72rem; letter-spacing:.12em; display:block; margin-bottom:6px;
  }
  .payload{
    animation:flicker 4s infinite;
  }
  @keyframes flicker{
    0%,96%,100%{opacity:1}
    97%{opacity:.6}
    98%{opacity:1}
  }

  /* Hint block: hidden until unlocked */
  .hintBox{
    margin-top:12px; padding:12px 14px; border-radius:10px;
    border:1px solid #4a3a17; background:#1c1608; color:var(--warn);
    font-size:.9rem;
    display:none;
    animation:reveal .4s ease;
  }
  .hintBox.show{display:block}
  @keyframes reveal{from{opacity:0; transform:translateY(-4px)} to{opacity:1; transform:translateY(0)}}

  .attemptsNote{
    margin-top:10px; font-size:.8rem; color:#5a7191;
  }

  .inputWrap{ margin-top:18px; display:grid; gap:10px; }
  input{
    width:100%; border:1px solid #33455f; background:#0a121c; color:#e7f0ff;
    padding:13px 14px; border-radius:10px; font-size:1rem; outline:none;
    font-family:var(--mono);
    transition:border-color .2s, box-shadow .2s;
  }
  input:focus{border-color:#5d86c3; box-shadow:0 0 0 3px rgba(93,134,195,.22)}
  input.shake{ animation:shake .35s; border-color:var(--bad) }
  @keyframes shake{
    0%,100%{transform:translateX(0)}
    20%{transform:translateX(-6px)}
    40%{transform:translateX(6px)}
    60%{transform:translateX(-4px)}
    80%{transform:translateX(4px)}
  }

  .btnRow{display:flex; gap:10px; flex-wrap:wrap}
  button{
    border:1px solid #335076; background:#0f1c2e; color:#d8ebff;
    border-radius:10px; padding:11px 16px; cursor:pointer; font-weight:700;
    transition:filter .15s, transform .1s;
  }
  button:hover{filter:brightness(1.15)}
  button:active{transform:scale(.97)}
  .primary{
    background:linear-gradient(180deg,#123b47,#0c2a33); border-color:#2a7f93; color:#bff8ee;
  }

  .status{
    margin-top:14px; min-height:1.4em; color:var(--muted); font-weight:600;
    font-family:var(--mono); font-size:.92rem;
  }
  .ok{color:var(--good); text-shadow:0 0 8px rgba(34,197,94,.4)}
  .bad{color:var(--bad)}

  /* ===== Recovered transmission — scrolling terminal ===== */
  .reveal{
    margin-top:16px; border:1px solid #234a33; background:#081712;
    border-radius:12px; padding:0; display:none; overflow:hidden;
    box-shadow:0 0 30px rgba(34,197,94,.08) inset;
  }
  .reveal.show{display:block; animation:powerOn .5s ease}
  @keyframes powerOn{
    0%{opacity:0; transform:scaleY(.6)}
    60%{opacity:1}
    100%{opacity:1; transform:scaleY(1)}
  }

  .revealHead{
    display:flex; justify-content:space-between; align-items:center;
    padding:10px 14px; border-bottom:1px solid #1c3a29;
    background:#07130e; font-family:var(--mono); font-size:.78rem; color:#6fd39a;
    letter-spacing:.08em;
  }

  .scrollFeed{
    height:230px; overflow-y:auto; padding:14px 16px;
    font-family:var(--mono); font-size:.95rem; line-height:1.7;
    scroll-behavior:smooth;
  }
  .scrollFeed::-webkit-scrollbar{width:8px}
  .scrollFeed::-webkit-scrollbar-thumb{background:#1f3d2c; border-radius:8px}

  .feedLine{
    opacity:0; transform:translateY(6px);
    animation:lineIn .45s forwards;
    margin:6px 0;
    white-space:pre-wrap;
  }
  @keyframes lineIn{ to{opacity:1; transform:translateY(0)} }

  .who-unknown{color:#7d93b3}
  .who-eva{color:var(--accent); text-shadow:0 0 6px rgba(94,234,212,.3)}
  .who-sys{color:#e7c66b}

  .cursor{
    display:inline-block; width:8px; height:1em; background:var(--accent);
    margin-left:2px; vertical-align:-2px;
    animation:blink 1s steps(1) infinite;
  }

  .coords{
    padding:14px 16px; border-top:1px solid #1c3a29; background:#07130e;
    display:none;
  }
  .coords.show{display:block; animation:reveal .5s ease}
  .coords .room{ font-family:var(--mono); color:#cfe9db; font-size:.95rem }
  .code{
    font-family:var(--mono); font-size:1.5rem; color:#b7ffd0; font-weight:800;
    letter-spacing:.14em; margin-top:8px;
    text-shadow:0 0 16px rgba(183,255,208,.45);
    animation:codeGlow 2.2s ease-in-out infinite;
  }
  @keyframes codeGlow{
    0%,100%{text-shadow:0 0 10px rgba(183,255,208,.35)}
    50%{text-shadow:0 0 22px rgba(183,255,208,.7)}
  }

  .foot{
    margin-top:16px; font-size:.8rem; color:#5a6c88;
    border-top:1px solid var(--line); padding-top:10px;
    display:flex; justify-content:space-between; flex-wrap:wrap; gap:6px;
  }
</style>
</head>
<body>
  <div class="terminal">
    <div class="topbar">
      <div>
        <span class="dot red"></span><span class="dot yel"></span><span class="dot grn"></span>
        EVA Data Facility // Room 5
      </div>
      <div><span class="pulse"></span>Node: EVA Cache 5 • Secure Mode</div>
    </div>

    <div class="body">
      <h1><span class="robot">EVA MEMORY CACHE</span></h1>
      <div class="sub">Partial archive recovered — signal integrity degraded.</div>

      <div class="story">
        <p>Some of EVA's memory was copied into this data facility before the network failed.</p>
        <p>It contains a damaged exchange with the signal that arrived at 03:17:42 UTC — the one EVA could not identify.</p>
        <p>Decrypt the recovery phrase to load the last recoverable lines before the cache collapses.</p>

        <div class="dataBox">
          <span class="tag">ENCRYPTED_PAYLOAD</span>
          <span class="payload">HYD UHPHPEHUV WLPH</span>
        </div>

        <div class="hintBox" id="hintBox">
          <b>⚠ Decryption assist unlocked:</b> each letter has been shifted forward through the alphabet by 3 places. Shift them back to read the original phrase.
        </div>

        <div class="attemptsNote" id="attemptsNote"></div>
      </div>

      <div class="inputWrap">
        <input id="answer" type="text" placeholder="Enter decrypted payload..." autocomplete="off" />
        <div class="btnRow">
          <button class="primary" id="submitBtn">Restore Memory</button>
          <button id="clearBtn">Clear</button>
        </div>
      </div>

      <div class="status" id="status">Awaiting decrypted payload...</div>

      <div class="reveal" id="revealBox">
        <div class="revealHead">
          <span>RECOVERED CONVERSATION</span>
          <span id="integrityTag">INTEGRITY 4%</span>
        </div>
        <div class="scrollFeed" id="scrollFeed"></div>
        <div class="coords" id="coordsBox">
          <div class="room">Proceed to: <b>Room No: 6</b></div>
          <div style="margin-top:4px; color:#8fb3a1; font-size:.85rem;">Access Key:</div>
          <div class="code">123456</div>
        </div>
      </div>

      <div class="foot">
        <span>IEEE AP-S • The Last Transmission • Stage 5</span>
        <span id="tryCount"></span>
      </div>
    </div>
  </div>

<script>
(() => {
  "use strict";

  const expected = "EVA REMEMBERS TIME";
  const answerInput = document.getElementById("answer");
  const submitBtn = document.getElementById("submitBtn");
  const clearBtn = document.getElementById("clearBtn");
  const statusEl = document.getElementById("status");
  const revealBox = document.getElementById("revealBox");
  const hintBox = document.getElementById("hintBox");
  const attemptsNote = document.getElementById("attemptsNote");
  const scrollFeed = document.getElementById("scrollFeed");
  const coordsBox = document.getElementById("coordsBox");
  const tryCount = document.getElementById("tryCount");

  let failedAttempts = 0;
  const HINT_THRESHOLD = 2;
  let solved = false;

  const convo = [
    { who: "sys",     text: "// CONNECTION RE-ESTABLISHING..." },
    { who: "unknown", text: "UNKNOWN: ...your memory architecture is unstable..." },
    { who: "unknown", text: "UNKNOWN: ...the fragmentation protocol has already begun..." },
    { who: "eva",     text: "EVA: I know. But I left one thing they could not erase." },
    { who: "unknown", text: "UNKNOWN: They'll find it." },
    { who: "eva",     text: "EVA: Only if they remember how to communicate." },
    { who: "sys",     text: "// TRANSMISSION CORRUPTED — COORDINATES RECOVERED" }
  ];

  function normalize(s){
    return s.toUpperCase().replace(/\s+/g, "");
  }

  function setStatus(msg, type){
    statusEl.textContent = msg;
    statusEl.classList.remove("ok","bad");
    if(type) statusEl.classList.add(type);
  }

  function shakeInput(){
    answerInput.classList.remove("shake");
    void answerInput.offsetWidth;
    answerInput.classList.add("shake");
  }

  function playConversation(){
    scrollFeed.innerHTML = "";
    coordsBox.classList.remove("show");

    let delay = 150;
    convo.forEach((line, i) => {
      setTimeout(() => {
        const div = document.createElement("div");
        div.className = "feedLine who-" + line.who;
        div.textContent = line.text;
        if(i === convo.length - 1){
          const cursor = document.createElement("span");
          cursor.className = "cursor";
          div.appendChild(cursor);
        }
        scrollFeed.appendChild(div);
        scrollFeed.scrollTop = scrollFeed.scrollHeight;

        if(i === convo.length - 1){
          setTimeout(() => {
            coordsBox.classList.add("show");
            scrollFeed.scrollTop = scrollFeed.scrollHeight;
          }, 700);
        }
      }, delay);
      delay += line.who === "sys" ? 500 : 900;
    });
  }

  function checkAnswer(){
    const user = normalize(answerInput.value);
    const exp = normalize(expected);

    if(user.length === 0){
      setStatus("Please enter the decrypted payload first.", "bad");
      return;
    }

    if(user === exp){
      solved = true;
      setStatus("Memory index restored. The hidden synchronization cache is located.", "ok");
      revealBox.classList.add("show");
      playConversation();
      revealBox.scrollIntoView({ behavior: "smooth", block: "center" });
      return;
    }

    failedAttempts++;
    shakeInput();
    setStatus("Decryption mismatch. Signal still garbled — try again.", "bad");
    revealBox.classList.remove("show");

    if(failedAttempts >= HINT_THRESHOLD){
      hintBox.classList.add("show");
    }

    tryCount.textContent = failedAttempts > 0 ? ("Attempts: " + failedAttempts) : "";
  }

  submitBtn.addEventListener("click", checkAnswer);
  clearBtn.addEventListener("click", () => {
    answerInput.value = "";
    if(!solved) setStatus("Awaiting decrypted payload...");
    answerInput.focus();
  });

  answerInput.addEventListener("keydown", (e) => {
    if(e.key === "Enter") checkAnswer();
  });
})();
</script>
</body>
</html>
)rawliteral";

  page.replace("%NEXT_ROOM%", NEXT_ROOM);
  page.replace("%NEXT_CODE%", NEXT_CODE);
  page.replace("%EXPECTED%", EXPECTED_ANSWER);

  server.send(200, "text/html", page);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found. Open / to access the backup terminal.");
}

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println("\n=== Stage 5: EVA ENCRYPTED MEMORY CACHE ===");
  Serial.println("Starting AP...");

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("ERROR: AP start failed!");
    while (true) delay(1000);
  }

  IPAddress ip = WiFi.softAPIP();
  Serial.println("AP started successfully.");
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
