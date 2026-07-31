#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ===========================================================
// Stage 5 - EVA Encrypted Memory Recovery Terminal
// ESP32 AP + captive portal + embedded story + decryption challenge
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

// ---------- Captive portal ----------
const byte DNS_PORT = 53;
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);

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
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0" />
<title>Stage 5 • EVA Recovery Terminal</title>
<style>
  :root{
    --bg:#171310;
    --paper:#ece1c4;
    --paper-dim:#ddd0a8;
    --paper-edge:#c7b78c;
    --ink:#241c12;
    --ink-soft:#5f5340;
    --red:#8a2b23;
    --green:#3d5940;
    --tape:#e3cf6f;
    --serif: Georgia, 'Times New Roman', serif;
    --mono: 'Courier New', ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
    --hand: 'Segoe Print', 'Bradley Hand', 'Comic Sans MS', cursive;
  }
  *{box-sizing:border-box}
  html,body{max-width:100%; overflow-x:hidden}
  body{
    margin:0; min-height:100vh; background:var(--bg);
    display:flex; justify-content:center; padding:28px 14px;
    font-family:var(--serif); color:var(--ink);
  }
  body::before{
    content:""; position:fixed; inset:0; pointer-events:none; z-index:0;
    background:
      radial-gradient(ellipse at 50% 0%, rgba(255,255,255,0.05), transparent 55%),
      repeating-linear-gradient(115deg, rgba(255,255,255,0.012) 0px, transparent 2px, transparent 4px);
  }

  .page{ position:relative; width:100%; max-width:640px; z-index:1; }

  /* torn top edge */
  .sheet{
    position:relative;
    background:var(--paper);
    padding:34px 30px 26px;
    clip-path: polygon(
      0% 9px, 4% 2px, 8% 8px, 12% 0px, 16% 6px, 20% 1px, 24% 7px, 28% 2px,
      32% 8px, 36% 1px, 40% 6px, 44% 0px, 48% 8px, 52% 2px, 56% 7px, 60% 1px,
      64% 8px, 68% 0px, 72% 6px, 76% 2px, 80% 8px, 84% 0px, 88% 7px, 92% 1px,
      96% 6px, 100% 0px,
      100% 100%, 0% 100%
    );
  }
  .sheet::before{
    content:"";
    position:absolute; inset:0;
    background:
      radial-gradient(circle at 15% 20%, rgba(120,100,60,0.06), transparent 40%),
      radial-gradient(circle at 85% 75%, rgba(120,100,60,0.05), transparent 45%);
    pointer-events:none;
  }
  /* tractor-feed holes */
  .sheet::after{
    content:"";
    position:absolute; top:0; bottom:0; left:6px; width:8px;
    background-image: radial-gradient(circle at center, var(--bg) 3.5px, transparent 4px);
    background-size: 8px 20px; background-repeat: repeat-y;
    opacity:.9;
  }
  .holesRight{
    content:"";
    position:absolute; top:0; bottom:0; right:6px; width:8px;
    background-image: radial-gradient(circle at center, var(--bg) 3.5px, transparent 4px);
    background-size: 8px 20px; background-repeat: repeat-y;
    opacity:.9;
  }

  .letterhead{
    display:flex; justify-content:space-between; align-items:flex-start;
    border-bottom:2px solid var(--ink); padding-bottom:10px; margin-bottom:18px;
    font-family:var(--mono); font-size:.72rem; letter-spacing:.06em; color:var(--ink-soft);
    flex-wrap:wrap; gap:6px;
  }
  .caseNo{ color:var(--ink); font-weight:700; }
  .liveTag{ display:flex; align-items:center; gap:6px; }
  .liveDot{
    width:7px; height:7px; border-radius:50%; background:var(--red);
    animation:pulse 1.6s ease-in-out infinite;
  }
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}

  h1{
    font-family:var(--mono); font-weight:700; font-size:1.4rem;
    letter-spacing:.05em; text-transform:uppercase; margin:0 0 4px;
    border-left:6px solid var(--ink); padding-left:12px;
  }
  .sub{
    font-family:var(--mono); font-size:.78rem; color:var(--ink-soft);
    letter-spacing:.04em; margin:0 0 20px; padding-left:18px;
  }

  .story p{ margin:0 0 12px; line-height:1.65; font-size:1rem; }
  .redact{
    background:var(--ink); color:var(--ink); border-radius:2px;
    padding:0 6px; user-select:none;
  }

  .exhibit{
    position:relative; margin:22px 4px 22px 4px;
    background:var(--paper-dim);
    border:1px solid var(--paper-edge);
    padding:16px 16px 14px;
    transform:rotate(-1deg);
    box-shadow:3px 3px 0 rgba(36,28,18,.12);
  }
  .tapeL, .tapeR{
    position:absolute; top:-10px; width:52px; height:22px;
    background:var(--tape); opacity:.65;
    box-shadow:0 1px 2px rgba(0,0,0,.15);
  }
  .tapeL{ left:14px; transform:rotate(-6deg); }
  .tapeR{ right:14px; transform:rotate(5deg); }
  .exhibitTag{
    font-family:var(--mono); font-size:.66rem; letter-spacing:.12em;
    color:var(--ink-soft); text-transform:uppercase; margin-bottom:8px; display:block;
  }
  .payload{
    font-family:var(--mono); font-size:1.05rem; letter-spacing:.08em;
    color:var(--ink); word-break:break-word;
  }

  .note{
    position:relative; margin:0 0 18px 4px; max-width:340px;
    background:#f2e7a6; padding:12px 14px 14px;
    font-family:var(--hand); font-size:1.05rem; color:#3a3115;
    transform:rotate(1deg);
    box-shadow:2px 2px 0 rgba(36,28,18,.10);
    display:none;
  }
  .note.show{ display:block; }
  .note b{ display:block; font-family:var(--mono); font-size:.68rem; letter-spacing:.08em;
    color:var(--red); text-transform:uppercase; margin-bottom:4px; font-weight:700; }

  .attemptsNote{ font-family:var(--mono); font-size:.75rem; color:var(--ink-soft); margin:0 0 14px; }

  .fillLine{ margin:22px 0 6px; }
  .fillLabel{
    font-family:var(--mono); font-size:.78rem; letter-spacing:.08em;
    color:var(--ink-soft); text-transform:uppercase; display:block; margin-bottom:6px;
  }
  input{
    width:100%; border:none; border-bottom:2px dotted var(--ink-soft);
    background:transparent; color:var(--ink);
    padding:8px 2px; font-size:1.05rem; outline:none;
    font-family:var(--mono);
  }
  input:focus{ border-bottom-color:var(--ink); }
  input.shake{ animation:shake .35s; }
  @keyframes shake{
    0%,100%{transform:translateX(0)}
    20%{transform:translateX(-6px)}
    40%{transform:translateX(6px)}
    60%{transform:translateX(-4px)}
    80%{transform:translateX(4px)}
  }

  .btnRow{ display:flex; gap:12px; flex-wrap:wrap; margin-top:16px; }
  button{
    font-family:var(--mono); font-weight:700; letter-spacing:.05em; text-transform:uppercase;
    font-size:.82rem; background:var(--paper); color:var(--ink);
    border:2px solid var(--ink); padding:10px 16px; cursor:pointer;
    box-shadow:3px 3px 0 var(--ink);
    transition:transform .08s, box-shadow .08s;
  }
  button:active{ transform:translate(2px,2px); box-shadow:1px 1px 0 var(--ink); }
  .primary{ background:var(--ink); color:var(--paper); box-shadow:3px 3px 0 var(--ink-soft); }
  .primary:active{ box-shadow:1px 1px 0 var(--ink-soft); }

  .status{
    margin-top:16px; min-height:1.4em; font-family:var(--mono); font-size:.85rem;
    color:var(--ink-soft); word-break:break-word;
  }
  .status.ok{ color:var(--green); font-weight:700; }
  .status.bad{ color:var(--red); font-weight:700; }

  .reveal{
    margin-top:20px; display:none;
    border-top:2px dashed var(--ink-soft); padding-top:16px;
  }
  .reveal.show{ display:block; }
  .revealHead{
    display:flex; justify-content:space-between; align-items:center;
    font-family:var(--mono); font-size:.7rem; letter-spacing:.1em; color:var(--ink-soft);
    text-transform:uppercase; margin-bottom:10px; flex-wrap:wrap; gap:4px;
  }
  .scrollFeed{
    max-height:230px; overflow-y:auto; overflow-x:hidden;
    background:var(--paper-dim); border:1px solid var(--paper-edge);
    padding:14px 16px; font-family:var(--mono); font-size:.92rem; line-height:1.7;
  }
  .scrollFeed::-webkit-scrollbar{width:8px}
  .scrollFeed::-webkit-scrollbar-thumb{background:var(--paper-edge)}
  .feedLine{
    opacity:0; transform:translateY(4px);
    animation:lineIn .4s forwards; margin:5px 0;
    white-space:pre-wrap; word-break:break-word;
  }
  @keyframes lineIn{ to{opacity:1; transform:translateY(0)} }
  .who-unknown{ color:var(--ink-soft); }
  .who-eva{ color:var(--green); font-weight:700; }
  .who-sys{ color:var(--red); }
  .cursor{
    display:inline-block; width:8px; height:12px; background:var(--ink);
    margin-left:2px; vertical-align:-2px; animation:blink 1s steps(1) infinite;
  }
  @keyframes blink{ 50%{opacity:0} }

  .coords{ margin-top:14px; display:none; }
  .coords.show{ display:block; }
  .stampWrap{
    display:inline-block; border:3px solid var(--red); color:var(--red);
    padding:8px 16px; transform:rotate(-3deg);
    font-family:var(--mono); font-weight:700; letter-spacing:.08em;
  }
  .coords .room{ font-size:.8rem; margin-bottom:4px; }
  .coords .code{
    font-size:1.5rem; letter-spacing:.16em;
  }

  .foot{
    margin-top:24px; padding-top:12px; border-top:1px solid var(--paper-edge);
    display:flex; justify-content:space-between; flex-wrap:wrap; gap:6px;
    font-family:var(--mono); font-size:.68rem; color:var(--ink-soft); letter-spacing:.04em;
  }

  @media (max-width:480px){
    body{ padding:16px 6px; }
    .sheet{ padding:28px 16px 20px; }
    h1{ font-size:1.1rem; padding-left:9px; border-left-width:5px; }
    .exhibit{ margin-left:0; margin-right:0; }
    .note{ max-width:100%; }
    .stampWrap{ padding:6px 10px; }
    .coords .code{ font-size:1.2rem; }
  }
</style>
</head>
<body>
  <div class="page">
    <div class="sheet">
      <div class="holesRight"></div>

      <div class="letterhead">
        <div><span class="caseNo">CASE FILE NO. 05</span> — EVA DATA RECOVERY UNIT</div>
        <div class="liveTag"><span class="liveDot"></span>SIGNAL LIVE</div>
      </div>

      <h1>Memory cache recovered</h1>
      <div class="sub">Partial archive · integrity degraded · Room 5</div>

      <div class="story">
        <p>Some of EVA's memory was copied into this data facility before the network failed.</p>
        <p>It contains a damaged exchange with the signal that arrived at <span class="redact">03:17:42 UTC</span> — the one EVA could not identify.</p>
        <p>Decrypt the recovery phrase to load the last recoverable lines before the cache collapses.</p>
      </div>

      <div class="exhibit">
        <div class="tapeL"></div><div class="tapeR"></div>
        <span class="exhibitTag">Exhibit A — intercepted fragment</span>
        <div class="payload">HYD UHPHPEHUV WLPH</div>
      </div>

      <div class="note" id="hintBox">
        <b>Margin note</b>
        Each letter's been pushed forward 3 places in the alphabet. Push them back to read it straight.
      </div>

      <div class="attemptsNote" id="attemptsNote"></div>

      <div class="fillLine">
        <span class="fillLabel">Decrypted phrase</span>
        <input id="answer" type="text" placeholder="Type the decoded payload..." autocomplete="off" />
      </div>

      <div class="btnRow">
        <button class="primary" id="submitBtn">Submit ►</button>
        <button id="clearBtn">Clear</button>
      </div>

      <div class="status" id="status">Awaiting decrypted payload...</div>

      <div class="reveal" id="revealBox">
        <div class="revealHead">
          <span>Recovered conversation</span>
          <span id="integrityTag">Integrity 4%</span>
        </div>
        <div class="scrollFeed" id="scrollFeed"></div>
        <div class="coords" id="coordsBox">
          <div class="stampWrap">
            <div class="room">Proceed to Room No. 6</div>
            <div class="code">123456</div>
          </div>
        </div>
      </div>

      <div class="foot">
        <span>IEEE AP-S · The Last Transmission · Stage 5</span>
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

// Any URL the browser/OS doesn't recognize -> redirect to the terminal.
// This is what makes the phone's captive-portal probe pop the browser
// automatically (Apple /hotspot-detect.html, Android /generate_204,
// Windows /connecttest.txt, etc. all land here and get bounced to "/").
void handleNotFound() {
  server.sendHeader("Location", String("http://") + apIP.toString(), true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println("\n=== Stage 5: EVA ENCRYPTED MEMORY CACHE ===");
  Serial.println("Starting AP...");

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
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

  // Captive portal: answer every DNS lookup with our own IP
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Web server + captive portal started.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  delay(2);
}
