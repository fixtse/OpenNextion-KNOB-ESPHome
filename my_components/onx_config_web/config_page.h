#pragma once
namespace esphome::onx_config_web {
static const char CONFIG_PAGE[] = R"ONXHTML(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Knob · Entity configuration</title>
<style>
:root{font-family:"Trebuchet MS",Arial,sans-serif;color:#172b46;background:#f6f8fc;font-size:16px;--blue:#205eaa;--line:#d9e2ef}*{box-sizing:border-box}body{margin:0}button,input{font:inherit}button{cursor:pointer;border:1px solid var(--line);background:white;color:inherit;padding:10px 15px;border-radius:8px}button:hover{border-color:var(--blue)}button:focus-visible,input:focus-visible{outline:3px solid #76aef0;outline-offset:3px}button:disabled{cursor:wait;opacity:.6}.primary{background:var(--blue);color:white;border-color:var(--blue)}header{padding:28px 32px;background:#e8f0fb;border-bottom:1px solid var(--line);display:flex;gap:20px;justify-content:space-between;align-items:center}h1{font-size:28px;margin:0 0 7px;letter-spacing:-.5px}p{line-height:1.55;margin:0}header p{color:#48627e}main{max-width:1100px;margin:auto;display:grid;grid-template-columns:210px minmax(0,1fr);gap:32px;padding:28px 24px}nav{display:flex;flex-direction:column;gap:7px}nav button{text-align:left;display:flex;align-items:center;gap:12px;min-height:60px}nav b{display:grid;place-items:center;border:1px solid var(--line);border-radius:50%;width:30px;height:30px;flex-shrink:0}nav span{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-size:13px}nav button[aria-current=true]{background:#e8f0fb;border-color:var(--blue)}.toolbar{display:flex;justify-content:space-between;align-items:center;gap:15px;margin-bottom:20px}h2{font-size:23px;margin:0}.bindings{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin:20px 0}.binding{background:white;padding:18px;border:1px solid var(--line);border-radius:10px;min-width:0}.binding.active{border:2px solid var(--blue);padding:17px}.binding h3{font-size:15px;font-weight:normal;color:#48627e;margin:0 0 12px}.binding strong{display:block;overflow-wrap:anywhere;min-height:42px;line-height:1.4}.binding small{display:block;color:#48627e;overflow-wrap:anywhere;margin:7px 0 15px;min-height:20px}.binding button{padding:7px 12px;font-size:14px}.quiet{background:transparent;border-color:transparent;color:#48627e}.search-head{display:flex;justify-content:space-between;gap:10px;align-items:center;margin-top:28px}.search-head h3{font-size:18px;margin:0}.hint{font-size:14px;color:#48627e;margin:12px 0}.search{width:100%;margin:16px 0 12px;padding:13px;border:1px solid #9cb1ca;border-radius:8px;background:white}.filters{display:flex;gap:7px;flex-wrap:wrap;margin-bottom:16px}.filters button{padding:7px 11px;font-size:13px}.filters button[aria-pressed=true]{background:#dceafb;border-color:var(--blue);color:#143e70}.results{display:flex;flex-direction:column;gap:0;border-top:1px solid var(--line)}.result{padding:15px 7px;border:0;border-bottom:1px solid var(--line);border-radius:0;background:transparent;text-align:left;display:flex;align-items:center;justify-content:space-between;gap:12px}.result strong{display:block;font-size:16px}.result small{display:block;margin-top:5px;color:#48627e;overflow-wrap:anywhere}.result em{font-size:12px;font-style:normal;color:#48627e;white-space:nowrap}.result:hover{background:#e8f0fb}.result.selected{background:#e0edfd}.empty{padding:25px 0;color:#48627e}.status{min-height:25px;font-size:14px;margin-top:10px;color:#305579}.status.error{color:#a33426}.more{margin-top:16px;width:100%}footer{border-top:1px solid var(--line);padding:18px 24px;text-align:center;font-size:13px;color:#48627e}.counter{font-size:13px;color:#48627e}#manual{margin-bottom:14px}#manual button{font-size:14px}.sr{position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0,0,0,0)}@media(max-width:720px){header{padding:22px 18px;display:block}header button{margin-top:16px}main{display:block;padding:18px}nav{flex-direction:row;overflow:auto;padding:3px 3px 12px;margin-bottom:16px}nav button{min-height:44px;padding:6px;flex-shrink:0}nav span{display:none}.bindings{gap:10px}.binding{padding:12px}.binding.active{padding:11px}h1{font-size:25px}.toolbar{align-items:flex-start}.result em{white-space:normal;max-width:85px}}@media(prefers-reduced-motion:reduce){*{scroll-behavior:auto}}
</style>
<header><div><h1>Make the knob yours.</h1><p>Choose what you see, and what a press does.</p></div><button id="refresh">Refresh from Home Assistant</button></header>
<section style="padding:16px 24px;background:#fff;border-bottom:1px solid #d9e2ef"><label for="ha-url">Home Assistant address</label> <input id="ha-url" type="url" placeholder="http://homeassistant.local:8123" style="max-width:100%;width:320px"> <button id="ha-connect">Connect to Home Assistant</button> <button id="ha-disconnect">Disconnect browser</button><p class="hint">Entity lists load directly into this browser. Only your selected entities are saved on the knob.</p></section>
<main><nav id="slots" aria-label="Device slots"></nav><section aria-label="Slot configuration"><div class="toolbar"><h2 id="slot-title">Slot 1</h2><button class="primary" id="save" disabled>Save slot</button></div>
<p class="hint">Open an entity on the knob, then press to run its selected action. Holding still returns to the list.</p>
<div class="bindings"><article class="binding active" id="display-binding"><h3>Show on the display</h3><strong id="entity-name">Empty slot</strong><small id="entity-id"></small><button id="choose-entity">Choose entity</button><button class="quiet" id="clear">Clear</button></article>
<article class="binding" id="action-binding"><h3>When pressed</h3><strong id="action-name">Device default</strong><small id="action-id">Use the entity’s normal controls</small><button id="choose-action">Choose action</button><button class="quiet" id="default">Default</button><button class="quiet" id="none">No action</button></article></div>
<label for="entity-type">Display type</label> <select id="entity-type"></select>
<details style="margin-top:16px"><summary>Standby weather</summary><select id="standby-weather" aria-label="Standby weather"></select> <button id="save-weather">Save standby weather</button></details>
<p class="status" id="status" role="status" aria-live="polite">Connecting to the knob…</p>
<div class="search-head"><h3 id="search-title">Find an entity</h3><span class="counter" id="count"></span></div>
<label class="sr" for="search">Search by name, entity ID, or room</label><input id="search" class="search" placeholder="Search by name, entity ID, or room" autocomplete="off" maxlength="128">
<div class="filters" id="filters" aria-label="Filter by entity type"></div><div id="manual"></div><div class="results" id="results" aria-live="polite"></div><button class="more" id="more" hidden>Show more</button>
<p class="hint" id="action-hint" hidden>Choose an automation or script. Automation conditions are checked before its actions run.</p>
</section></main><footer>Saved on your knob. Changes apply without reflashing.</footer>
<script src="/knob/ha.js"></script>
<script>
const TOKEN='__ONX_TOKEN__';
const $=id=>document.getElementById(id);
const ha=new OnxHA.Client();let weather='';
let slots=Array.from({length:10},()=>({entity:'',name:'',action:'',dirty:false})),active=0,mode='entity',domain='',next=-1,revision=0,debounce;
const domains=[['','All'],['sensor','Sensors'],['binary_sensor','Binary sensors'],['light','Lights'],['switch','Switches'],['climate','Climate'],['media_player','Media players'],['timer','Timers'],['weather','Weather'],['cover','Covers'],['actions','Automations & scripts']];
function message(text,error=false){$('status').textContent=text;$('status').classList.toggle('error',error)}
async function rpc(values){
 const response=await fetch('/knob/api',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Onx-Token':TOKEN},body:new URLSearchParams(values)});
 const queued=await response.json();if(!response.ok||queued.error)throw Error(queued.error||'Device connection failed');
 for(let i=0;i<100;i++){
  await new Promise(resolve=>setTimeout(resolve,100));
  const result=await fetch('/knob/result?ticket='+queued.ticket,{cache:'no-store'});if(result.status===202)continue;
  const body=await result.json();if(!result.ok||body.error)throw Error(body.error||'The device could not save this change');return body;
 }throw Error('The knob did not respond. Check its connection and retry.');
}
function element(tag,text,className){const el=document.createElement(tag);el.textContent=text;if(className)el.className=className;return el}
function actionLabel(action){return !action?'Device default':action==='none'?'No action':action.startsWith('script.')?'Run script':'Run automation'}
function render(){
 $('slots').replaceChildren(...slots.map((slot,index)=>{const button=element('button','');button.append(element('b',String(index+1)),element('span',slot.name||slot.entity||'Empty slot'));button.setAttribute('aria-label','Slot '+(index+1)+': '+(slot.name||slot.entity||'Empty'));button.setAttribute('aria-current',String(index===active));button.onclick=()=>{active=index;render();search()};return button}));
 const slot=slots[active];$('slot-title').textContent='Slot '+(active+1);$('entity-name').textContent=slot.name||slot.entity||'Empty slot';$('entity-id').textContent=slot.entity;
 $('action-name').textContent=slot.actionName||actionLabel(slot.action);$('action-id').textContent=slot.action&&slot.action!=='none'?slot.action:slot.action==='none'?'Ignore a short press':'Use the entity’s normal controls';
 renderTypes(slot);$('save').disabled=!slot.dirty;$('display-binding').classList.toggle('active',mode==='entity');$('action-binding').classList.toggle('active',mode==='action');$('action-hint').hidden=mode!=='action';$('search-title').textContent=mode==='action'?'Find an automation or script':'Find an entity';
 $('filters').replaceChildren(...(mode==='action'?[['actions','All actions'],['automation','Automations'],['script','Scripts']]:domains).map(([value,title])=>{const button=element('button',title);button.setAttribute('aria-pressed',String(value===domain));button.onclick=()=>{domain=value;render();search()};return button}));
}
function renderTypes(slot){
 const d=slot.entity.split('.')[0];const choices=d==='light'?['Light (On/Off)','Dimmable Light','Color Temperature Light','RGB Light']:d==='switch'?['Switch','Plug']:d==='cover'?['Cover (Preset Off)','Cover (Preset On)']:['Automatic'];
 $('entity-type').replaceChildren(...choices.map(value=>{const option=element('option',value);option.value=value==='Automatic'?'':value;return option}));
 $('entity-type').value=slot.typeOption||(choices[0]==='Automatic'?'':choices[0]);
 if(!$('entity-type').value&&choices[0]!=='Automatic')$('entity-type').value=choices[0];
}
$('entity-type').onchange=()=>{slots[active].typeOption=$('entity-type').value;dirty()};
function weatherOptions(){
 const items=ha.items.filter(i=>i.entity.startsWith('weather.'));if(weather&&!items.some(i=>i.entity===weather))items.push({entity:weather,name:weather});
 $('standby-weather').replaceChildren(...[{entity:'',name:'None'},...items].map(item=>{const option=element('option',item.name||item.entity);option.value=item.entity;return option}));$('standby-weather').value=weather;
}
$('save-weather').onclick=async()=>{try{const entity=$('standby-weather').value,item=ha.items.find(i=>i.entity===entity)||{};await rpc({op:'weather',entity,name:item.name||entity,area:item.area||''});weather=entity;message('Standby weather saved.')}catch(error){message(error.message,true)}};
function dirty(){slots[active].dirty=true;render();message('Changes not saved')}
function choose(item){const slot=slots[active];if(mode==='entity'){slot.entity=item.entity;slot.name=item.name||item.entity;slot.area=item.area||'';slot.typeOption=item.type||''}else{slot.action=item.entity;slot.actionName=item.name||actionLabel(item.entity)}dirty();search()}
function isManualEntity(value){return /^[a-z0-9_]+\.[a-z0-9_]+$/.test(value)&&value.length<=128&&/^(sensor|binary_sensor|light|switch|climate|media_player|timer|weather|cover|automation|script)\./.test(value)&&(mode!=='action'||/^(automation|script)\./.test(value))}
async function search(append=false){
 const mine=++revision;const offset=append?next:0;if(!append)$('results').replaceChildren(element('p','Searching…','empty'));$('more').hidden=true;$('manual').replaceChildren();
 const query=$('search').value.trim();
 try{const data=OnxHA.search(ha.items,query,domain,offset);
  if(!ha.ready&&!query)message('Connect to Home Assistant to load the entity catalogue in this browser.');if(mine!==revision)return;
  if(!append)$('results').replaceChildren();$('count').textContent=data.total+' matches';
  if(!data.items.length&&!append)$('results').append(element('p','No matches. Try a different name, room, or entity type.','empty'));
  for(const item of data.items){const row=element('button','','result');const text=element('span','');text.append(element('strong',item.name||item.entity),element('small',item.entity));row.append(text,element('em',item.area||item.entity.split('.')[0]));row.classList.toggle('selected',(mode==='entity'?slots[active].entity:slots[active].action)===item.entity);row.onclick=()=>choose(item);$('results').append(row)}
  next=data.next;$('more').hidden=next<0;
  if(isManualEntity(query)&&!data.items.some(item=>item.entity===query)){const button=element('button','Use entity ID: '+query);button.onclick=()=>choose({entity:query,name:query});$('manual').append(button)}
 }catch(error){if(mine!==revision)return;$('results').replaceChildren(element('p','Could not load entities.','empty'));message(error.message,true)}
}
function selectMode(value){mode=value;domain=value==='action'?'actions':'';$('search').value='';render();search();$('search').focus()}
$('choose-entity').onclick=()=>selectMode('entity');$('choose-action').onclick=()=>selectMode('action');
$('default').onclick=()=>{slots[active].action='';slots[active].actionName='';dirty()};$('none').onclick=()=>{slots[active].action='none';slots[active].actionName='';dirty()};
$('clear').onclick=()=>{slots[active]={entity:'',name:'',action:'',dirty:true};render();message('Slot cleared. Save to apply.')};
$('search').oninput=()=>{clearTimeout(debounce);++revision;debounce=setTimeout(()=>search(),250)};$('more').onclick=()=>search(true);
$('save').onclick=async()=>{const index=active,slot={...slots[active]};$('save').disabled=true;message('Saving…');try{await rpc({op:'save',slot:index,entity:slot.entity,action:slot.action,name:slot.name||'',area:slot.area||'',action_name:slot.actionName||'',type:slot.typeOption||''});if(slots[index].entity===slot.entity&&slots[index].action===slot.action)slots[index].dirty=false;render();message('Slot '+(index+1)+' saved. Home Assistant may reconnect briefly.')}catch(error){render();message(error.message,true)}};
async function refreshCatalogue(){
 const button=$('refresh');button.disabled=true;message('Loading directly from Home Assistant into this browser…');
 try{await ha.load();weatherOptions();await search();message(ha.items.length+' entities loaded in this browser. The knob was not refreshed or restarted.');}
 catch(error){message(error.message,true)}finally{button.disabled=false}
}
$('refresh').onclick=()=>refreshCatalogue();
$('ha-connect').onclick=async()=>{try{sessionStorage.setItem('onx.draft',JSON.stringify(slots));await ha.login($('ha-url').value)}catch(error){message(error.message,true)}};
$('ha-disconnect').onclick=()=>{ha.disconnect();sessionStorage.removeItem('onx.draft');search();message('Browser disconnected from Home Assistant. Saved knob settings are unchanged.')};
(async()=>{try{
 const data=await rpc({op:'state'});slots=data.slots.map(slot=>({...slot,dirty:false}));weather=data.weather||'';$('ha-url').value=data.ha_url||'';
 await ha.finishLogin();const draft=JSON.parse(sessionStorage.getItem('onx.draft')||'null');sessionStorage.removeItem('onx.draft');if(Array.isArray(draft)&&draft.length===10)slots=draft;
 const auth=ha.read('onx.ha.auth');if(auth)$('ha-url').value=auth.base;
 render();weatherOptions();message('Connect to Home Assistant to choose entities. Current assignments are saved on the knob.');await search();
 if(auth)await refreshCatalogue();
}catch(error){render();message(error.message,true)}})();
</script></html>
)ONXHTML";
}
