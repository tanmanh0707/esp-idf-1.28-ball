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
h1{font-size:1.4rem;font-weight:700;color:#1a1a2e;margin-bottom:20px}
.group{background:#fff;border-radius:14px;padding:20px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.08)}
.group-title{font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:.1em;color:#888;margin-bottom:16px}
.field{margin-bottom:14px}
.field:last-child{margin-bottom:0}
label{display:block;font-size:.85rem;font-weight:500;color:#444;margin-bottom:6px}
select,input{display:block;width:100%;padding:10px 12px;font-size:.95rem;color:#222;background:#fff;border:1.5px solid #ddd;border-radius:8px;outline:none;transition:border-color .2s;-webkit-appearance:none;appearance:none}
select:focus,input:focus{border-color:#4361ee}
.row{display:flex;gap:8px}
.row select{flex:1;min-width:0}
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
</style>
</head>
<body>
<div class="container">
<h1>Device Setup</h1>
<div class="group">
<div class="group-title">WiFi</div>
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
<button class="btn-save" id="saveBtn" onclick="doSave()">Save &amp; Reboot</button>
<div class="msg" id="msg"></div>
</div>
<script>
function esc(s){return s.replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;');}
async function doScan(){
  const s=document.getElementById('ssid');
  s.innerHTML='<option>Scanning…</option>';
  try{
    const r=await fetch('/scan');
    const nets=await r.json();
    if(!nets.length){s.innerHTML='<option value="">No networks found</option>';return;}
    s.innerHTML=nets.map(n=>`<option value="${esc(n.ssid)}">${esc(n.ssid)}&nbsp;&nbsp;${n.rssi} dBm${n.auth?' &#128274;':''}</option>`).join('');
  }catch(e){s.innerHTML='<option value="">Scan failed — retry</option>';}
}
function togglePw(btn){
  const inp=document.getElementById('pw');
  inp.type=inp.type==='password'?'text':'password';
  btn.innerHTML=inp.type==='password'?'&#128065;':'&#128584;';
}
async function doSave(){
  const ssid=document.getElementById('ssid').value;
  const pw=document.getElementById('pw').value;
  if(!ssid){show('Please select a WiFi network.',false);return;}
  const btn=document.getElementById('saveBtn');
  btn.disabled=true;btn.textContent='Saving…';
  try{
    const r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:pw})});
    const d=await r.json();
    if(d.ok){show('Saved! Device is rebooting — reconnect to your WiFi shortly.',true);}
    else{show(d.error||'Save failed.',false);btn.disabled=false;btn.textContent='Save & Reboot';}
  }catch(e){show('Connection error.',false);btn.disabled=false;btn.textContent='Save & Reboot';}
}
function show(msg,ok){const el=document.getElementById('msg');el.textContent=msg;el.className='msg '+(ok?'ok':'err');}
doScan();
</script>
</body>
</html>)html";
