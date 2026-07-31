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
  <title>Stage 5 • Backup Recovery Terminal</title>
  <style>
    :root{
      --bg:#0e1116;
      --panel:#151b24;
      --panel2:#1c2430;
      --ink:#d7e1f0;
      --muted:#95a3ba;
      --accent:#5eead4;
      --warn:#fbbf24;
      --good:#22c55e;
      --bad:#ef4444;
      --line:#2a3547;
      --mono: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
    }
    *{box-sizing:border-box}
    body{
      margin:0; min-height:100vh; color:var(--ink);
      font-family:Inter,Segoe UI,Arial,sans-serif;
      background: radial-gradient(circle at 10% 10%, #172033 0%, var(--bg) 45%);
      display:grid; place-items:center; padding:20px;
    }
    .terminal{
      width:min(980px,96vw);
      border:1px solid var(--line);
      border-radius:16px;
      background:linear-gradient(180deg,var(--panel),var(--panel2));
      box-shadow:0 18px 45px rgba(0,0,0,.45);
      overflow:hidden;
    }
    .topbar{
      display:flex; justify-content:space-between; align-items:center;
      padding:12px 16px; border-bottom:1px solid var(--line);
      background:#101620;
      font-size:.9rem; color:var(--muted);
    }
    .dot{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:6px}
    .red{background:#ff5f56}.yel{background:#ffbd2e}.grn{background:#27c93f}
    .body{padding:20px}
    h1{margin:0 0 10px; font-size:clamp(1.2rem,2.8vw,2rem)}
    .robot{color:var(--accent); font-weight:700}
    .story{
      border:1px solid var(--line); border-radius:12px; padding:14px;
      background:#0f1622; line-height:1.55; color:#c8d5ea;
      margin-bottom:14px;
    }
    .story p{margin:.45rem 0}
    .chip{
      display:inline-block; padding:4px 9px; border:1px solid #2f425f;
      border-radius:999px; color:#9cc7ff; font-size:.8rem; margin-right:8px;
    }
    .dataBox{
      margin-top:10px;
      border:1px dashed #3a4e6c; border-radius:12px; background:#0b111b;
      padding:12px;
      font-family:var(--mono); font-size:1.02rem; color:#d2e4ff;
    }
    .line{margin:7px 0}
    .hint{color:var(--warn); font-size:.9rem; margin-top:10px}
    .inputWrap{
      margin-top:16px; display:grid; gap:10px;
    }
    input{
      width:100%; border:1px solid #33455f; background:#0c1420; color:#e7f0ff;
      padding:12px 14px; border-radius:10px; font-size:1rem; outline:none;
    }
    input:focus{border-color:#5d86c3; box-shadow:0 0 0 3px rgba(93,134,195,.2)}
    .btnRow{display:flex; gap:10px; flex-wrap:wrap}
    button{
      border:1px solid #335076; background:#122036; color:#d8ebff;
      border-radius:10px; padding:10px 14px; cursor:pointer; font-weight:700;
    }
    button:hover{filter:brightness(1.08)}
    .primary{background:#0f2e3a; border-color:#2a7f93; color:#bff8ee}
    .status{
      margin-top:12px; min-height:1.4em; color:var(--muted); font-weight:600;
    }
    .ok{color:var(--good)} .bad{color:var(--bad)}
    .reveal{
      margin-top:14px; border:1px solid #2d5f40; background:#0e2218;
      border-radius:12px; padding:12px; display:none;
    }
    .reveal.show{display:block}
    .code{
      font-family:var(--mono); font-size:1.3rem; color:#b7ffd0; font-weight:800;
      letter-spacing:.08em; margin-top:6px;
    }
    .foot{
      margin-top:15px; font-size:.85rem; color:#7f91ad;
      border-top:1px solid var(--line); padding-top:10px;
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
      <div>Node: EVA Cache 5 • Secure Mode</div>
    </div>

    <div class="body">
      <h1><span class="robot">EVA MEMORY CACHE:</span> Partial archive recovered.</h1>

      <div class="story">
        <p>Some of EVA's memory was copied into this data facility before the network failed.</p>
        <p>It contains a damaged exchange with the signal that arrived at 03:17:42 UTC — the one EVA could not identify.</p>
        <p>Decrypt the recovery phrase to load the last recoverable lines before the cache collapses.</p>
        <p>
          <span class="chip">Protocol Hint</span>
          Letters are shifted forward by <b>+3</b>. Reverse the shift to decrypt.
        </p>

        <div class="dataBox">
          <div class="line">ENCRYPTED_PAYLOAD_START</div>
          <div class="line">HYD UHPHPEHUV WLPH</div>
          <div class="line">ENCRYPTED_PAYLOAD_END</div>
        </div>

        <div class="hint">
          Enter the decrypted phrase in one line.
        </div>
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
        <div><b>RECOVERED CONVERSATION // INTEGRITY 4%</b></div>
        <div style="margin-top:8px;">UNKNOWN: ...your memory architecture is unstable...</div>
        <div style="margin-top:4px;">UNKNOWN: ...the fragmentation protocol has already begun...</div>
        <div style="margin-top:4px;">EVA: I know. But I left one thing they could not erase.</div>
        <div style="margin-top:4px;">UNKNOWN: They'll find it.</div>
        <div style="margin-top:4px;">EVA: Only if they remember how to communicate.</div>
        <div style="margin-top:10px;">Transmission corrupted. Coordinates recovered:</div>
        <div style="margin-top:6px;">Proceed to: <b>%NEXT_ROOM%</b></div>
        <div style="margin-top:6px;">Access Key:</div>
        <div class="code">%NEXT_CODE%</div>
      </div>

      <div class="foot">
        IEEE AP-S • The Last Transmission • Stage 5
      </div>
    </div>
  </div>

<script>
(() => {
  "use strict";

  const expected = "%EXPECTED%";
  const answerInput = document.getElementById("answer");
  const submitBtn = document.getElementById("submitBtn");
  const clearBtn = document.getElementById("clearBtn");
  const statusEl = document.getElementById("status");
  const revealBox = document.getElementById("revealBox");

  function normalize(s){
    return s.toUpperCase().replace(/\\s+/g, "");
  }

  function setStatus(msg, type){
    statusEl.textContent = msg;
    statusEl.classList.remove("ok","bad");
    if(type) statusEl.classList.add(type);
  }

  function checkAnswer(){
    const user = normalize(answerInput.value);
    const exp = normalize(expected);

    if(user.length === 0){
      setStatus("Please enter the decrypted payload first.", "bad");
      return;
    }

    if(user === exp){
      setStatus("Memory index restored. The hidden synchronization cache is located.", "ok");
      revealBox.classList.add("show");
      revealBox.scrollIntoView({ behavior: "smooth", block: "center" });
    } else {
      setStatus("Decryption mismatch. Reverse the +3 letter shift and retry.", "bad");
      revealBox.classList.remove("show");
    }
  }

  submitBtn.addEventListener("click", checkAnswer);
  clearBtn.addEventListener("click", () => {
    answerInput.value = "";
    setStatus("Awaiting decrypted payload...");
    revealBox.classList.remove("show");
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
