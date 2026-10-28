#pragma once
#include <Arduino.h>

// Página de control servida en http://ngen-ko.local (o la IP del equipo).
// Todo va en un solo archivo para no depender de internet ni de SPIFFS.
static const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="es"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ngen-ko</title>
<style>
:root{--bg:#f4f6f3;--card:#fff;--fg:#1d2b22;--mut:#6b7a70;--acc:#2f7d4f;--warn:#b5651d;--err:#b33a3a;--line:#dfe5e0}
@media(prefers-color-scheme:dark){:root{--bg:#121714;--card:#1b221e;--fg:#e4ece6;--mut:#8d9b92;--acc:#4fae73;--line:#2b3530}}
*{box-sizing:border-box}body{margin:0;font:15px system-ui,sans-serif;background:var(--bg);color:var(--fg)}
header{padding:16px;display:flex;justify-content:space-between;align-items:baseline}
h1{margin:0;font-size:20px}small,.mut{color:var(--mut)}
main{max-width:760px;margin:auto;padding:0 16px 32px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px;margin-bottom:14px}
.seg{display:flex;gap:6px}.seg button{flex:1}
button{font:inherit;padding:8px 12px;border-radius:8px;border:1px solid var(--line);background:var(--card);color:var(--fg);cursor:pointer}
button.on{background:var(--acc);border-color:var(--acc);color:#fff}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(160px,1fr));gap:10px}
.zone{border:1px solid var(--line);border-radius:8px;padding:10px}
.zone h3{margin:0 0 6px;font-size:15px;display:flex;justify-content:space-between}
.bar{height:8px;background:var(--line);border-radius:4px;overflow:hidden;position:relative;margin:6px 0}
.bar i{display:block;height:100%;background:var(--acc)}
.bar b{position:absolute;top:-2px;width:2px;height:12px;background:var(--fg)}
.st-regando{color:var(--acc)}.st-pausa,.st-esperando{color:var(--warn)}.st-falla-sensor{color:var(--err)}
label{display:flex;justify-content:space-between;align-items:center;gap:8px;margin:6px 0}
input{font:inherit;width:90px;padding:5px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--fg)}
input[type=checkbox]{width:auto}input.wide{width:200px}
h2{font-size:16px;margin:0 0 10px}
#msg{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:var(--fg);color:var(--bg);padding:8px 14px;border-radius:8px;display:none}
</style></head><body>
<header><h1>Ngen-ko</h1><small id="info"></small></header>
<main>
<div class="card"><h2>Modo</h2><div class="seg" id="modes">
<button data-m="0">Apagado</button><button data-m="1">Manual</button><button data-m="2">Automático</button></div>
<p class="mut" id="manualHint" style="display:none">Modo manual: toca una zona para abrir o cerrar su válvula. <button id="allBtn">Todas</button></p>
</div>
<div class="card"><h2>Zonas</h2><div class="grid" id="zones"></div></div>
<div class="card"><h2>Ajustes</h2>
<div class="grid" id="thresholds"></div>
<label>Histéresis (%)<input id="hysteresis" type="number" min="0" max="50"></label>
<label>Tiempo máximo de riego por zona (min, 0 = sin límite)<input id="maxRunMin" type="number" min="0" max="600"></label>
<label>Pausa tras tiempo máximo (min)<input id="cooldownMin" type="number" min="0" max="1440"></label>
<label>Regar una zona a la vez<input id="oneZoneAtATime" type="checkbox"></label>
<label>Solo regar en horario<input id="windowEnabled" type="checkbox"></label>
<label>Desde (hora)<input id="windowStartHour" type="number" min="0" max="23"></label>
<label>Hasta (hora)<input id="windowEndHour" type="number" min="0" max="23"></label>
<label>Zona horaria (POSIX)<input id="tz" class="wide"></label>
<button id="saveCfg" class="on">Guardar ajustes</button>
</div>
<div class="card"><h2>Calibración de sensores</h2>
<p class="mut">Con el sensor seco al aire pulsa "Seco"; luego sumérgelo en agua hasta la línea y pulsa "Mojado".</p>
<div class="grid" id="calib"></div></div>
<div class="card"><h2>WiFi</h2>
<label>Red<input id="ssid" class="wide"></label>
<label>Clave<input id="pass" class="wide" type="password"></label>
<button id="saveWifi">Guardar y reiniciar</button></div>
</main><div id="msg"></div>
<script>
const $=id=>document.getElementById(id);let S=null,editing=false;
const toast=t=>{const m=$('msg');m.textContent=t;m.style.display='block';setTimeout(()=>m.style.display='none',2500)};
async function api(path,body){const r=await fetch(path,body?{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)}:{});
const j=await r.json();if(!r.ok){toast(j.error||'Error');throw j}return j}
function fmtMin(s){return s<60?s+' s':Math.floor(s/60)+' min'}
function render(s){S=s;
$('info').textContent=(s.time||'sin hora')+' · '+(s.wifi.connected?s.wifi.ip+' ('+s.wifi.rssi+' dBm)':'red propia');
document.querySelectorAll('#modes button').forEach(b=>b.classList.toggle('on',+b.dataset.m===s.mode));
$('manualHint').style.display=s.mode===1?'block':'none';
$('zones').innerHTML=s.zones.map((z,i)=>`<div class="zone" data-z="${i}" style="${s.mode===1?'cursor:pointer':''}">
<h3>Zona ${i+1}<span>${z.on?'💧 ON':'OFF'}</span></h3>
<div class="bar"><i style="width:${Math.max(z.moisture,0)}%"></i><b style="left:${z.threshold}%"></b></div>
<div>${z.moisture<0?'Sensor: ERR':'Humedad '+z.moisture+'%'} <small>/ umbral ${z.threshold}%</small></div>
<small class="st-${z.status.replace(' ','-')}">${z.status}${z.on?' · '+fmtMin(z.onForSec):''}</small></div>`).join('');
document.querySelectorAll('.zone').forEach(el=>el.onclick=()=>{if(S.mode!==1)return;const i=+el.dataset.z;
api('/api/relay',{zone:i,on:!S.zones[i].on}).then(render)});
$('calib').innerHTML=s.zones.map((z,i)=>`<div class="zone"><h3>Zona ${i+1}<small>${z.mv} mV</small></h3>
<small>Seco ${z.airMv} · Mojado ${z.waterMv}</small><div class="seg" style="margin-top:6px">
<button onclick="cal(${i},'air')">Seco</button><button onclick="cal(${i},'water')">Mojado</button></div></div>`).join('');
if(!editing)fillForm(s)}
function fillForm(s){const c=s.config;
for(const k of ['hysteresis','maxRunMin','cooldownMin','windowStartHour','windowEndHour','tz'])$(k).value=c[k];
$('oneZoneAtATime').checked=c.oneZoneAtATime;$('windowEnabled').checked=c.windowEnabled;
$('thresholds').innerHTML=s.zones.map((z,i)=>`<div class="zone"><h3>Zona ${i+1}</h3>
<label>Umbral %<input id="th${i}" type="number" min="0" max="100" value="${z.threshold}"></label>
<label>Habilitada<input id="en${i}" type="checkbox" ${z.enabled?'checked':''}></label></div>`).join('');
if(!$('ssid').value)$('ssid').value=s.wifi.ssid}
function cal(i,p){api('/api/calibrate',{zone:i,point:p}).then(s=>{render(s);toast('Calibrado')})}
document.querySelectorAll('#modes button').forEach(b=>b.onclick=()=>api('/api/mode',{mode:+b.dataset.m}).then(render));
$('allBtn').onclick=()=>api('/api/relay',{zone:-1,on:!S.zones.every(z=>z.on)}).then(render);
document.querySelector('main').addEventListener('focusin',e=>{if(e.target.closest('#thresholds')||e.target.closest('.card label'))editing=true});
$('saveCfg').onclick=()=>{const n=k=>+$(k).value;api('/api/config',{hysteresis:n('hysteresis'),maxRunMin:n('maxRunMin'),
cooldownMin:n('cooldownMin'),windowStartHour:n('windowStartHour'),windowEndHour:n('windowEndHour'),tz:$('tz').value,
oneZoneAtATime:$('oneZoneAtATime').checked,windowEnabled:$('windowEnabled').checked,
zones:S.zones.map((_,i)=>({threshold:n('th'+i),enabled:$('en'+i).checked}))}).then(s=>{editing=false;render(s);toast('Ajustes guardados')})};
$('saveWifi').onclick=()=>api('/api/wifi',{ssid:$('ssid').value,pass:$('pass').value}).then(()=>toast('Reiniciando... reconéctate a tu red'));
const poll=()=>api('/api/state').then(render).catch(()=>{}).finally(()=>setTimeout(poll,2000));poll();
</script></body></html>)rawliteral";
