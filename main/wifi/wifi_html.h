#pragma once

static const char SETTINGS_HTML[] = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Device Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#f0f2f5;min-height:100vh;padding:24px 16px}
.container{max-width:480px;margin:0 auto}
.header{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:16px}
h1{font-size:1.4rem;font-weight:700;color:#1a1a2e}
.fw-ver{font-size:.75rem;color:#aaa;white-space:nowrap;padding-left:8px}
.tabs{display:flex;gap:4px;background:#e4e6eb;border-radius:10px;padding:4px;margin-bottom:16px}
.tab-btn{flex:1;padding:9px;border:none;border-radius:7px;cursor:pointer;font-size:.9rem;font-weight:600;color:#666;background:transparent;transition:.15s}
.tab-btn.active{background:#fff;color:#1a1a2e;box-shadow:0 1px 3px rgba(0,0,0,.12)}
.tab-panel{display:none}
.tab-panel.active{display:block}
.group{background:#fff;border-radius:14px;padding:20px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.08)}
.group-title{font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:.1em;color:#888;margin-bottom:16px}
.field{margin-bottom:14px}
.field:last-child{margin-bottom:0}
label{display:block;font-size:.85rem;font-weight:500;color:#444;margin-bottom:6px}
select,input[type=text],input[type=url],input[type=password],input[type=number]{display:block;width:100%;padding:10px 12px;font-size:.95rem;color:#222;background:#fff;border:1.5px solid #ddd;border-radius:8px;outline:none;transition:border-color .2s;-webkit-appearance:none;appearance:none}
select:focus,input:focus{border-color:#4361ee}
.row{display:flex;gap:8px;align-items:center}
.row select{flex:1;min-width:0}
.ssid-display{flex:1;padding:10px 12px;font-size:.95rem;color:#333;background:#f8f8f8;border:1.5px solid #ddd;border-radius:8px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.btn-sm{padding:10px 14px;background:#f4f4f4;border:1.5px solid #ddd;border-radius:8px;cursor:pointer;font-size:.85rem;color:#555;white-space:nowrap;flex-shrink:0}
.btn-sm:hover{background:#e8e8e8}
.pw-wrap{position:relative}
.pw-wrap input{padding-right:44px}
.eye{position:absolute;right:12px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;color:#999;font-size:1rem;line-height:1;padding:0}
.btn-save{display:block;width:100%;padding:14px;margin-top:10px;background:#4361ee;color:#fff;border:none;border-radius:10px;font-size:1rem;font-weight:600;cursor:pointer;transition:background .2s}
.btn-save:hover:not(:disabled){background:#3451d1}
.btn-save:disabled{background:#aaa;cursor:default}
.msg{margin-top:14px;padding:12px 16px;border-radius:8px;font-size:.875rem;text-align:center;display:none}
.msg.ok{display:block;background:#d4edda;color:#155724}
.msg.err{display:block;background:#f8d7da;color:#721c24}
.hint{font-size:.75rem;color:#aaa;margin-top:4px}
.hidden{display:none!important}
/* OTA */
.drop-zone{border:2px dashed #ddd;border-radius:12px;padding:32px 20px;text-align:center;cursor:pointer;transition:border-color .2s;margin-bottom:14px}
.drop-zone:hover,.drop-zone.drag{border-color:#4361ee;background:#f5f7ff}
.drop-zone input[type=file]{display:none}
.drop-icon{font-size:2rem;margin-bottom:8px}
.drop-label{font-size:.9rem;color:#666}
.drop-hint{font-size:.75rem;color:#aaa;margin-top:4px}
.file-info{font-size:.85rem;color:#333;margin-bottom:14px;padding:10px 12px;background:#f8f8f8;border:1.5px solid #ddd;border-radius:8px;display:none}
.progress-wrap{margin-bottom:14px;display:none}
.progress-track{background:#e9ecef;border-radius:99px;height:12px;overflow:hidden}
.progress-fill{height:100%;background:#4361ee;border-radius:99px;width:0%;transition:width .2s}
.progress-label{text-align:center;font-size:.8rem;color:#666;margin-top:6px}
</style>
</head>
<body>
<div class="container">

<div class="header">
  <h1>Device Setup</h1>
  <span class="fw-ver" id="fw-ver"></span>
</div>

<!-- Tab bar -->
<div class="tabs">
  <button class="tab-btn active" onclick="switchTab('settings')">Settings</button>
  <button class="tab-btn"        onclick="switchTab('upgrade')">Upgrade</button>
</div>

<!-- ══════════════════════ SETTINGS TAB ══════════════════════ -->
<div class="tab-panel active" id="tab-settings">

<!-- WiFi group -->
<div class="group">
<div class="group-title">WiFi</div>
<div id="wifi-saved" class="hidden">
<div class="field">
<label>Current Network</label>
<div class="row">
<div class="ssid-display" id="saved-ssid-text"></div>
<button class="btn-sm" onclick="showWifiForm()">Change</button>
</div>
</div>
</div>
<div id="wifi-form" class="hidden">
<div class="field">
<label>Network</label>
<div class="row">
<select id="ssid"><option value="">-- Tap Scan --</option></select>
<button class="btn-sm" onclick="doScan()">&#8635; Scan</button>
</div>
</div>
<div class="field">
<label>Password</label>
<div class="pw-wrap">
<input type="password" id="pw" placeholder="Enter WiFi password">
<button class="eye" type="button" onclick="togglePw(this)">&#128065;</button>
</div>
</div>
</div>
</div>

<!-- Home Screen group -->
<div class="group">
<div class="group-title">Home Screen</div>
<div class="field">
<label>Background duration (seconds)</label>
<input type="number" id="bg_dur" min="5" max="3600" placeholder="60">
<div class="hint">How long to show background + clock before switching to tasks.</div>
</div>
<div class="field">
<label>Task list duration (seconds)</label>
<input type="number" id="task_dur" min="5" max="300" placeholder="15">
<div class="hint">How long to show the full-screen task list.</div>
</div>
</div>

<!-- Google Calendar group -->
<div class="group">
<div class="group-title">Google Calendar</div>
<div class="field">
<label>Apps Script URL</label>
<input type="url" id="gcal_url" placeholder="https://script.google.com/macros/s/…/exec">
</div>
<div class="field">
<label>Device ID</label>
<input type="text" id="gcal_device" placeholder="e.g. esp32-001">
</div>
<div class="field">
<label>API Key</label>
<div class="pw-wrap">
<input type="password" id="gcal_key" placeholder="Your secret key">
<button class="eye" type="button" onclick="togglePw(this)">&#128065;</button>
</div>
</div>
<div class="field">
<label>Days ahead</label>
<input type="number" id="gcal_days" min="1" max="90" placeholder="7">
<div class="hint">Number of upcoming days to fetch (1–90). Leave all fields empty to disable.</div>
</div>
</div>

<!-- Google Tasks group -->
<div class="group">
<div class="group-title">Google Tasks</div>
<div class="field">
<label>Apps Script URL</label>
<input type="url" id="gtask_url" placeholder="https://script.google.com/macros/s/…/exec">
</div>
<div class="field">
<label>Auth Token</label>
<div class="pw-wrap">
<input type="password" id="gtask_token" placeholder="Your secret token">
<button class="eye" type="button" onclick="togglePw(this)">&#128065;</button>
</div>
<div class="hint">Leave both fields empty to disable task fetching.</div>
</div>
</div>

<button class="btn-save" id="saveBtn" onclick="doSave()">Save &amp; Reboot</button>
<div class="msg" id="msg-settings"></div>
</div><!-- /tab-settings -->

<!-- ══════════════════════ UPGRADE TAB ══════════════════════ -->
<div class="tab-panel" id="tab-upgrade">
<div class="group">
<div class="group-title">Firmware Upgrade</div>

<div class="drop-zone" id="drop-zone" onclick="document.getElementById('fw-file').click()"
     ondragover="event.preventDefault();this.classList.add('drag')"
     ondragleave="this.classList.remove('drag')"
     ondrop="onDrop(event)">
  <input type="file" id="fw-file" accept=".bin" onchange="onFileSelected(this.files[0])">
  <div class="drop-icon">&#128190;</div>
  <div class="drop-label">Click or drag firmware .bin here</div>
  <div class="drop-hint">Use <code>firmware_ota.bin</code> from the merge script</div>
</div>

<div class="file-info" id="file-info"></div>

<div class="progress-wrap" id="progress-wrap">
  <div class="progress-track"><div class="progress-fill" id="progress-fill"></div></div>
  <div class="progress-label" id="progress-label">0%</div>
</div>

<button class="btn-save" id="upgradeBtn" onclick="doUpgrade()" disabled>Upgrade</button>
<div class="msg" id="msg-upgrade"></div>
</div>
</div><!-- /tab-upgrade -->

</div><!-- /container -->

<script>
var wifiChanged = false;
var fwFile = null;

// ── Tab switching ─────────────────────────────────────────────────────────────
function switchTab(name) {
  document.querySelectorAll('.tab-btn').forEach(function(b,i){
    b.classList.toggle('active', (i===0&&name==='settings')||(i===1&&name==='upgrade'));
  });
  document.getElementById('tab-settings').classList.toggle('active', name==='settings');
  document.getElementById('tab-upgrade').classList.toggle('active', name==='upgrade');
}

// ── Firmware version ──────────────────────────────────────────────────────────
(async function loadVersion(){
  try {
    const r = await fetch('/version');
    const d = await r.json();
    if (d.version) document.getElementById('fw-ver').textContent = 'v' + d.version;
  } catch(e) {}
})();

// ── Settings: populate on load ────────────────────────────────────────────────
function esc(s){return s.replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;');}

(async function init(){
  try {
    const r = await fetch('/settings');
    const d = await r.json();
    if (d.wifi && d.wifi.ssid && d.wifi.ssid !== '') {
      document.getElementById('saved-ssid-text').textContent = d.wifi.ssid;
      document.getElementById('wifi-saved').classList.remove('hidden');
    } else {
      document.getElementById('wifi-form').classList.remove('hidden');
      wifiChanged = true;
    }
    if (d.gcal) {
      document.getElementById('gcal_url').value    = d.gcal.url    || '';
      document.getElementById('gcal_device').value = d.gcal.device || '';
      document.getElementById('gcal_key').value    = d.gcal.key    || '';
      document.getElementById('gcal_days').value   = d.gcal.days   || '7';
    }
    if (d.gtask) {
      document.getElementById('gtask_url').value   = d.gtask.url   || '';
      document.getElementById('gtask_token').value = d.gtask.token || '';
    }
    if (d.homescreen) {
      document.getElementById('bg_dur').value   = d.homescreen.bg_dur   || '60';
      document.getElementById('task_dur').value = d.homescreen.task_dur || '15';
    }
  } catch(e) {
    document.getElementById('wifi-form').classList.remove('hidden');
    wifiChanged = true;
  }
})();

function showWifiForm() {
  document.getElementById('wifi-saved').classList.add('hidden');
  document.getElementById('wifi-form').classList.remove('hidden');
  wifiChanged = true;
  doScan();
}

async function doScan() {
  const s = document.getElementById('ssid');
  s.innerHTML = '<option>Scanning…</option>';
  try {
    const r    = await fetch('/scan');
    const nets = await r.json();
    if (!nets.length) { s.innerHTML = '<option value="">No networks found</option>'; return; }
    s.innerHTML = nets.map(function(n){
      return '<option value="'+esc(n.ssid)+'">'+esc(n.ssid)+'&nbsp;&nbsp;'+n.rssi+' dBm'+(n.auth?' &#128274;':'')+' </option>';
    }).join('');
  } catch(e) { s.innerHTML = '<option value="">Scan failed — retry</option>'; }
}

function togglePw(btn) {
  var inp = btn.previousElementSibling;
  inp.type = inp.type === 'password' ? 'text' : 'password';
  btn.innerHTML = inp.type === 'password' ? '&#128065;' : '&#128584;';
}

async function doSave() {
  var body = {
    bg_dur:       document.getElementById('bg_dur').value.trim(),
    task_dur:     document.getElementById('task_dur').value.trim(),
    gcal_url:     document.getElementById('gcal_url').value.trim(),
    gcal_device:  document.getElementById('gcal_device').value.trim(),
    gcal_key:     document.getElementById('gcal_key').value.trim(),
    gcal_days:    document.getElementById('gcal_days').value.trim(),
    gtask_url:    document.getElementById('gtask_url').value.trim(),
    gtask_token:  document.getElementById('gtask_token').value.trim()
  };
  if (wifiChanged) {
    var ssid = document.getElementById('ssid').value;
    if (!ssid) { showMsg('settings', 'Please select a WiFi network.', false); return; }
    body.ssid     = ssid;
    body.password = document.getElementById('pw').value;
  }
  var btn = document.getElementById('saveBtn');
  btn.disabled = true; btn.textContent = 'Saving…';
  try {
    var r = await fetch('/save', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(body)});
    var d = await r.json();
    if (d.ok) { showMsg('settings', 'Saved! Device is rebooting — reconnect to your WiFi shortly.', true); }
    else { showMsg('settings', d.error || 'Save failed.', false); btn.disabled = false; btn.textContent = 'Save & Reboot'; }
  } catch(e) { showMsg('settings', 'Connection error.', false); btn.disabled = false; btn.textContent = 'Save & Reboot'; }
}

// ── Upgrade tab ───────────────────────────────────────────────────────────────
function onDrop(e) {
  e.preventDefault();
  document.getElementById('drop-zone').classList.remove('drag');
  var f = e.dataTransfer.files[0];
  if (f) onFileSelected(f);
}

function onFileSelected(f) {
  if (!f) return;
  fwFile = f;
  var info = document.getElementById('file-info');
  info.style.display = 'block';
  info.textContent = f.name + '  (' + (f.size / 1024).toFixed(1) + ' KB)';
  document.getElementById('upgradeBtn').disabled = false;
  document.getElementById('progress-wrap').style.display = 'none';
  document.getElementById('progress-fill').style.width = '0%';
  document.getElementById('progress-label').textContent = '0%';
  showMsg('upgrade', '', false);  // clear
  document.getElementById('msg-upgrade').style.display = 'none';
}

function doUpgrade() {
  if (!fwFile) return;
  var btn = document.getElementById('upgradeBtn');
  btn.disabled = true;
  btn.textContent = 'Upgrading…';

  var wrap  = document.getElementById('progress-wrap');
  var fill  = document.getElementById('progress-fill');
  var label = document.getElementById('progress-label');
  wrap.style.display = 'block';
  fill.style.width = '0%';
  label.textContent = '0%';

  var xhr = new XMLHttpRequest();
  xhr.upload.onprogress = function(e) {
    if (!e.lengthComputable) return;
    var pct = Math.round(e.loaded / e.total * 100);
    fill.style.width  = pct + '%';
    label.textContent = pct + '%  (' + (e.loaded/1024).toFixed(0) + ' / ' + (e.total/1024).toFixed(0) + ' KB)';
  };
  xhr.onload = function() {
    try {
      var d = JSON.parse(xhr.responseText);
      if (d.ok) {
        fill.style.width = '100%'; label.textContent = '100%';
        showMsg('upgrade', 'Upgrade successful! Device is rebooting with new firmware.', true);
      } else {
        showMsg('upgrade', d.error || 'Upgrade failed.', false);
        btn.disabled = false; btn.textContent = 'Upgrade';
      }
    } catch(e) { showMsg('upgrade', 'Invalid response from device.', false); btn.disabled = false; btn.textContent = 'Upgrade'; }
  };
  xhr.onerror = function() {
    showMsg('upgrade', 'Connection lost during upload.', false);
    btn.disabled = false; btn.textContent = 'Upgrade';
  };
  xhr.open('POST', '/ota');
  xhr.setRequestHeader('Content-Type', 'application/octet-stream');
  xhr.send(fwFile);
}

function showMsg(tab, msg, ok) {
  var el = document.getElementById('msg-' + tab);
  el.textContent = msg;
  el.className   = 'msg ' + (ok ? 'ok' : 'err');
  if (msg) el.style.display = 'block'; else el.style.display = 'none';
}
</script>
</body>
</html>)html";
