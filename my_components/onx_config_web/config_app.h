#pragma once
namespace esphome::onx_config_web {
static const char CONFIG_APP_JS[] = R"ONXJS(
(()=>{
'use strict';
const $=id=>document.getElementById(id),ha=new OnxHA.Client(),DRAFT='onx.config.draft.v1';
const domains=[['','All'],['sensor','Sensors'],['binary_sensor','Binary sensors'],['light','Lights'],['switch','Switches'],['fan','Fan'],['number','Number'],['input_number','Input Number'],['select','Select'],['input_select','Input Select'],['scene','Scene'],['button','Button'],['input_button','Input Button'],['climate','Climate'],['media_player','Media players'],['timer','Timers'],['weather','Weather'],['cover','Covers'],['actions','Automations & scripts']];
const copy=value=>JSON.parse(JSON.stringify(value));
const emptyGestures=()=>({cw:'',ccw:'',holdCw:'',holdCcw:'',feedback:'',popup:'1',responses:'0',events:'0',step:'1'});
const empty=()=>({entity:'',name:'',area:'',action:'',actionName:'',typeOption:'',icon:'',iconOverride:'0',secondary:'',secondaryText:'{value}',sensorGraph:'0',timerHidden:'0',gestures:emptyGestures(),playlists:[]});
const MAX_SLOTS=20;
let visibleSlots=1;
let slots=Array.from({length:MAX_SLOTS},empty),saved=copy(slots),active=0,mode='entity',domain='',next=-1,revision=0,debounce,busy=false,initialized=false,device={},settings={},savedSettings={},savedWeather='',recovery=null,lastLoaded=null,statusBusy=false,previewBusy=false,liveViews=[],overlayTimer=0,lastPreviewAt=0,slotTab='display',playlistTarget=-1,gestureTarget='cw';
function element(tag,text,className){const e=document.createElement(tag);e.textContent=text;if(className)e.className=className;return e}
function message(text,error=false){$('status').textContent=text;$('status').classList.toggle('error',error)}
function safeRead(key){try{return JSON.parse(sessionStorage.getItem(key)||'null')}catch{return null}}
function dirtySlot(i){return JSON.stringify(slots[i])!==JSON.stringify(saved[i])}
function dirtySettings(){return JSON.stringify(settings)!==JSON.stringify(savedSettings)}
function hasChanges(){return slots.some((_,i)=>dirtySlot(i))||dirtySettings()}
function persistDraft(){if(!initialized)return;if(recovery&&hasChanges()){recovery=null;$('draft-notice').hidden=true}if(recovery)return;try{if(hasChanges())sessionStorage.setItem(DRAFT,JSON.stringify({schema:2,slots,settings}));else sessionStorage.removeItem(DRAFT)}catch{message('This browser could not store a recovery draft. Save your changes before closing.',true)}}
function actionLabel(action){return !action?'Device default':action==='none'?'No action':action.startsWith('script.')?'Run script':'Run automation'}
function nativePress(entity){return /^(fan|number|input_number|select|input_select|scene|button|input_button)\./.test(entity)}
function choices(slot){const d=slot.entity.split('.')[0];return d==='light'?['Light (On/Off)','Dimmable Light','Color Temperature Light','RGB Light']:d==='switch'?['Switch','Plug']:d==='cover'?['Cover (Preset Off)','Cover (Preset On)']:['Automatic']}
function slotPayload(index,slot){return {op:'save',slot:index,entity:slot.entity,action:slot.action,name:slot.name,area:slot.area,action_name:slot.actionName,type:slot.typeOption,icon:slot.icon,icon_override:slot.iconOverride,secondary:slot.secondary,secondary_text:slot.secondaryText,sensor_graph:slot.sensorGraph,timer_hidden:slot.timerHidden,gestures:JSON.stringify(slot.gestures),playlists:JSON.stringify(slot.playlists)}}
async function fetchJson(url,options={}){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),10000);
 try{const response=await fetch(url,{...options,cache:'no-store',signal:controller.signal});const body=await response.json();if(!response.ok||body.error)throw Error(body.error||'The knob could not complete this request');return {response,body}}
 catch(error){if(error.name==='AbortError')throw Error('The knob did not respond. Check its Wi-Fi connection and retry.');throw error}
 finally{clearTimeout(timer)}
}
async function rpc(values){
 const {body:queued}=await fetchJson('/knob/api',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Onx-Token':window.ONX_TOKEN},body:new URLSearchParams(values)});
 for(let i=0;i<60;i++){await new Promise(resolve=>setTimeout(resolve,150));const {response,body}=await fetchJson('/knob/result?ticket='+queued.ticket);if(response.status!==202)return body}
 throw Error('The knob did not confirm this change. Refresh status before retrying.');
}
function browserStatus(){const loggedIn=!!ha.read('onx.ha.auth');$('browser-status').textContent='Browser → Home Assistant: '+(ha.ready?'entity list loaded':loggedIn?'signed in':'disconnected');$('browser-status').dataset.ok=String(ha.ready);$('catalogue-time').textContent=lastLoaded?'Entity list loaded '+lastLoaded.toLocaleTimeString()+'. '+ha.items.length+' available entities.':'No entity list loaded in this browser.'}
function deviceStatus(online){$('device-status').textContent=online?'Knob: reachable':'Knob: unreachable';$('device-status').dataset.ok=String(online);$('ha-status').textContent='Knob → Home Assistant: '+(!online?'unknown':device.ha_connected?'connected':'not connected');$('ha-status').dataset.ok=String(online&&device.ha_connected);const info=$('device-info');info.replaceChildren();for(const [key,value] of [['Model','ONX2424G013'],['Firmware',device.firmware||'Unknown'],['Uptime',Number.isFinite(device.uptime_ms)?Math.floor(device.uptime_ms/60000)+' minutes':'Unknown'],['Wi-Fi signal',Number.isFinite(device.wifi_rssi_dbm)?Math.round(device.wifi_rssi_dbm)+' dBm':'Unavailable on this firmware'],['Wi-Fi disconnects',device.wifi_disconnects_since_boot??'Unknown'],['Last reset',device.reset_reason_text||'Unknown'],['Internal heap',Number.isFinite(device.internal_heap_free)?Math.round(device.internal_heap_free/1024)+' KB free / '+Math.round(device.internal_heap_minimum/1024)+' KB minimum':'Unavailable on this firmware'],['Artwork',device.artwork_error||device.artwork_status||'Unknown'],['Wi-Fi',online?(device.wifi_connected?'connected':'disconnected'):'unknown'],['Home Assistant',online?(device.ha_connected?'connected':'disconnected'):'unknown']])info.append(element('dt',key),element('dd',value))}
function page(name){document.querySelectorAll('[id^=page-]').forEach(e=>e.hidden=e.id!=='page-'+name);document.querySelectorAll('[data-page]').forEach(e=>{if(e.dataset.page===name)e.setAttribute('aria-current','page');else e.removeAttribute('aria-current')})}
document.querySelectorAll('[data-page]').forEach(e=>e.onclick=()=>page(e.dataset.page));
const svgNS='http://www.w3.org/2000/svg',measure=document.createElement('canvas').getContext('2d');
function svgNode(tag,attrs={}){const node=document.createElementNS(svgNS,tag);for(const [key,value] of Object.entries(attrs))node.setAttribute(key,String(value));return node}
function fitted(text,size,width){measure.font=size+'px Arial';let out=String(text);if(measure.measureText(out).width<=width)return out;while(out&&measure.measureText(out+'…').width>width)out=out.slice(0,-1);return out+'…'}
function drawPreview(slot){
 clearTimeout(overlayTimer);
 const svg=$('device-preview');let view=liveViews.find(item=>item.entity===slot.entity);
 if(view&&$('preview-screen').value==='main')view={...view,...view.carousel};
 svg.replaceChildren(svgNode('circle',{cx:120,cy:120,r:120,fill:'#000'}));
 svg.setAttribute('aria-label',(slot.name||slot.entity||'Empty slot')+' device display preview');
 const hex=color=>'#'+Number(color??0xffffff).toString(16).padStart(6,'0');
 const text=(content,y,size,width=196,color=0xffffff,x=0,wrap=false)=>{
  const words=String(content).split(' '),rows=wrap?['']:String(content).split('\n');
  if(wrap){measure.font=size+'px Arial';for(const word of words){const i=rows.length-1,candidate=(rows[i]+' '+word).trim();if(measure.measureText(candidate).width>width&&rows[i]&&rows.length<2)rows.push(word);else rows[i]=candidate}}
  for(const [i,row] of rows.slice(0,2).entries()){const line=svgNode('text',{x:120+x,y:y+size*.85+i*(size+2),'text-anchor':'middle','font-size':size,fill:hex(color)});line.textContent=fitted(row,size,width);svg.append(line)}
 };
 if(!view){text(slot.entity?'Save to load':'Choose an entity',88,18);if(slot.entity)text('live device data',112,16);$('preview-status').textContent=slot.entity?'No live view for this unsaved entity.':'Choose what this slot displays.';return}
 if(view.layout==='graph'){
  const points=view.graph||[],now=view.graph_now||0,values=points.map(p=>p.value),low=Math.min(...values),high=Math.max(...values);
  for(const y of [104,136,168])svg.append(svgNode('line',{x1:27,y1:y,x2:213,y2:y,stroke:'#344653','stroke-width':1}));
  for(let i=1;i<points.length;i++){const a=points[i-1],b=points[i];if(b.t-a.t>1200)continue;
   const x=p=>27+185-(now-p.t)*185/43200,y=p=>104+(high>low?64-(p.value-low)*64/(high-low):32);
   svg.append(svgNode('line',{x1:x(a),y1:y(a),x2:x(b),y2:y(b),stroke:settings.media_color||'#62B0FF','stroke-width':2}));}
 }
 if(view.layout==='media'&&view.progress>=0){
  svg.append(svgNode('circle',{cx:120,cy:120,r:117,fill:'none',stroke:'#344653','stroke-width':4}));
  const ring=svgNode('circle',{cx:120,cy:120,r:117,fill:'none',stroke:settings.media_color||hex(view.ring_color??0x62b0ff),'stroke-width':4,'stroke-linecap':'round',pathLength:1000,'stroke-dasharray':view.progress+' '+(1000-view.progress),transform:'rotate(-90 120 120)',class:'preview-ring','data-playing':String(!!view.playing),opacity:view.playing?1:.6});svg.append(ring);
 }
 if(view.icon&&/^mdi:[a-z0-9-]+$/.test(view.icon)){
  const size=view.layout==='media'?80:view.icon_size;
  const icon=svgNode('image',{href:'https://api.iconify.design/mdi/'+view.icon.slice(4)+'.svg?color=%23ffffff',x:120+(view.icon_x||0)-size/2,y:view.icon_y,width:size,height:size});svg.append(icon);
 }
 if(view.layout==='media'&&view.artwork){try{const url=new URL(view.artwork);if(['http:','https:'].includes(url.protocol))svg.append(svgNode('image',{href:url.href,x:70,y:18,width:100,height:100,preserveAspectRatio:'xMidYMid slice'}))}catch{}}
 if(view.layout==='media'&&view.overlay&&view.overlay_path){
  const paint=(kind,path)=>{const group=svgNode('g',{'data-transport-overlay':kind});group.append(svgNode('circle',{cx:120,cy:68,r:24,fill:'#000',opacity:.7}),svgNode('path',{d:path,transform:'translate(104 52)',fill:'#fff'}));svg.append(group);return group};
  const group=paint(view.overlay,view.overlay_path);
  if(view.overlay!=='pause'&&view.overlay_ms>0)overlayTimer=setTimeout(()=>{if(!group.isConnected)return;group.remove();if(view.paused&&view.pause_path)paint('pause',view.pause_path)},view.overlay_ms);
 }
 if(view.layout==='carousel')for(const [i,item] of (view.navigator||[]).entries()){
  if(!item.visible)continue;
  const angle=(item.offset||0)*17*Math.PI/180,x=120+102*Math.sin(angle),y=120+102*Math.cos(angle);
  if(item.selected)svg.append(svgNode('circle',{cx:x,cy:y,r:12,fill:'#20384E'}));
  if(/^mdi:[a-z0-9-]+$/.test(item.icon||''))svg.append(svgNode('image',{href:'https://api.iconify.design/mdi/'+item.icon.slice(4)+'.svg?color='+encodeURIComponent(item.selected?'#62B0FF':'#ffffff'),x:x-9,y:y-9,width:18,height:18,opacity:item.selected?1:.55}));
 }
 for(const [i,line] of (view.lines||[]).entries()){const name=i===4;text(name&&!slot.secondary?(slot.name||line.text):line.text,line.y,line.size,line.width,line.color,line.x||0,name&&view.layout!=='media')}
 $('preview-status').textContent=view.available?'Live device view':'Device view · waiting for entity data';
}
async function refreshPreview(){if(previewBusy||busy||!initialized||document.hidden||$('entity-picker').open)return;previewBusy=true;lastPreviewAt=Date.now();try{const data=await rpc({op:'preview',entity:slots[active].entity});liveViews=Array.isArray(data.slots)?data.slots:[];drawPreview(slots[active])}catch{$('preview-status').textContent='Live preview unavailable. Check the knob connection.'}finally{previewBusy=false}}
function preview(){const s=slots[active],d=s.entity.split('.')[0];drawPreview(s);
 const rotates={light:'Adjust the selected light control',cover:'Browse; hold and rotate to adjust position',climate:'Adjust the selected setting; hold and turn to choose a setting',fan:'Adjust speed; hold and turn for presets, oscillation or direction',number:'Adjust value within minimum, maximum and step',input_number:'Adjust value within minimum, maximum and step',select:'Choose an option',input_select:'Choose an option',media_player:'Adjust volume; press + turn for next/previous track; hold 0.5s and release for playlists'};
 $('rotate-hint').textContent=s.gestures.cw||s.gestures.ccw||s.gestures.events==='1'?'Run configured gesture actions':s.entity?(rotates[d]||'No adjustment; hold to return'):'Browse your devices';
 const presses={light:'Toggle power',cover:'Use the cover’s normal controls',climate:'Cycle supported HVAC modes; confirm a previewed option',fan:'Toggle power; confirm a previewed option',select:'Confirm option',input_select:'Confirm option',number:'Value sent automatically',input_number:'Value sent automatically',scene:'Activate scene',button:'Press button',input_button:'Press button',media_player:'Play/pause when loaded; run the selected action when idle',script:'Run script',automation:'Use the automation’s normal controls',sensor:'Send sensor press event',binary_sensor:'Send sensor press event',weather:'Read only',timer:'Read only'};
 $('press-hint').textContent=d==='media_player'?('Playing/paused: play/pause. Idle: '+(s.action==='none'?'no action':s.action?(s.actionName||actionLabel(s.action)):'choose an automation or script')):!s.entity?'Open the selected device':nativePress(s.entity)?presses[d]:s.action==='none'?'No action':s.action?(s.actionName||actionLabel(s.action)):presses[d]||'Toggle power';
}
function iconButton(label,kind){
 const b=element('button','','icon-button');b.type='button';b.title=label;b.setAttribute('aria-label',label);
 const paths={up:'M12 19V5m-6 6 6-6 6 6',down:'M12 5v14m-6-6 6 6 6-6',remove:'m6 6 12 12M18 6 6 18'};
 const svg=svgNode('svg',{viewBox:'0 0 24 24','aria-hidden':'true',focusable:'false'});svg.append(svgNode('path',{d:paths[kind]}));b.append(svg);return b;
}
function markSlotChanges(text){
 persistDraft();const count=slots.filter((_,i)=>dirtySlot(i)).length;
 $('save').disabled=busy||!initialized||!dirtySlot(active);$('save-all').disabled=busy||!initialized||!count;$('discard').disabled=busy||!count;
 $('unsaved-count').textContent=count?count+' unsaved slot'+(count===1?'':'s'):'';message(text);
}
function shortPlaylistName(value){
 let out='';const encoder=new TextEncoder();for(const char of value){if(encoder.encode(out+char).length>48)break;out+=char}return out;
}
function renderEditorTabs(){
 const media=slots[active].entity.startsWith('media_player.');$('slot-tab-playlists').hidden=!media;
 if(slotTab==='playlists'&&!media)slotTab='display';
 for(const button of document.querySelectorAll('[data-slot-tab]')){
  const chosen=button.dataset.slotTab===slotTab;button.setAttribute('aria-selected',String(chosen));button.tabIndex=chosen?0:-1;button.disabled=busy;
  $('slot-panel-'+button.dataset.slotTab).hidden=!chosen;
 }
}
for(const button of document.querySelectorAll('[data-slot-tab]')){
 button.onclick=()=>{slotTab=button.dataset.slotTab;renderEditorTabs()};
 button.onkeydown=event=>{
  if(!['ArrowLeft','ArrowRight','Home','End'].includes(event.key))return;event.preventDefault();
  const tabs=[...document.querySelectorAll('[data-slot-tab]')].filter(b=>!b.hidden&&!b.disabled),index=tabs.indexOf(button);
  const next=event.key==='Home'?0:event.key==='End'?tabs.length-1:(index+(event.key==='ArrowRight'?1:-1)+tabs.length)%tabs.length;
  if(tabs[next]){slotTab=tabs[next].dataset.slotTab;renderEditorTabs();tabs[next].focus()}
 };
}
function renderPlaylists(){
 const s=slots[active];$('playlist-options').hidden=!s.entity.startsWith('media_player.');$('playlist-count').textContent=s.playlists.length+' of 8';
 $('playlist-entries').replaceChildren();
 if(!s.playlists.length)$('playlist-entries').append(element('p','Add a playlist or station, then choose the action that starts it.','empty'));
 s.playlists.forEach((entry,index)=>{
  const row=element('article','','playlist-row'),head=element('div','','playlist-row-head'),tools=element('div','','playlist-row-tools');
  row.setAttribute('aria-label','Playlist '+(index+1));head.append(element('span',String(index+1),'playlist-order'));
  for(const [label,delta,icon] of [['Move up',-1,'up'],['Move down',1,'down'],['Remove',0,'remove']]){
   const b=iconButton(label+' playlist '+(index+1),icon);if(!delta)b.classList.add('danger');
   b.disabled=busy||!initialized||(delta<0&&index===0)||(delta>0&&index===s.playlists.length-1);
   b.onclick=()=>{if(delta)[s.playlists[index],s.playlists[index+delta]]=[s.playlists[index+delta],s.playlists[index]];else s.playlists.splice(index,1);changed()};tools.append(b);
  }
  head.append(tools);row.append(head);
  const fields=element('div','','playlist-fields'),nameField=element('label','Display name','field'),name=element('input','');
  name.value=entry.name;name.maxLength=48;name.placeholder='Use the chosen action’s name';name.setAttribute('aria-label','Playlist '+(index+1)+' name');name.disabled=busy||!initialized;
  name.oninput=()=>{entry.name=name.value;markSlotChanges('Playlist changes not saved.')};nameField.append(name);
  const action=element('div','','playlist-action'),selected=ha.items.find(i=>i.entity===entry.action);
  action.append(element('span','Runs','field-caption'),element('strong',entry.action?(selected?.name||entry.action):'No action selected'));
  if(entry.action)action.append(element('small',entry.action));
  const choose=element('button',entry.action?'Change action':'Choose action');choose.type='button';choose.dataset.choosePlaylist=String(index);
  choose.setAttribute('aria-label','Choose action for playlist '+(index+1));choose.disabled=busy||!initialized;choose.onclick=()=>selectMode('playlist',index);action.append(choose);
  fields.append(nameField,action);row.append(fields);$('playlist-entries').append(row);
 });
 $('add-playlist').disabled=busy||!initialized||s.playlists.length>=8;
}
$('add-playlist').onclick=()=>selectMode('playlist',slots[active].playlists.length);
function isActionPicker(){return mode==='action'||mode==='playlist'||mode==='gesture'}
function selectedPickerEntity(){const s=slots[active];return mode==='gesture'?s.gestures[gestureTarget]:mode==='gestureFeedback'?s.gestures.feedback:mode==='playlist'?s.playlists[playlistTarget]?.action:mode==='secondary'?s.secondary:mode==='action'?s.action:s.entity}
function closePicker(){if($('entity-picker').open)$('entity-picker').close()}
$('close-picker').onclick=closePicker;
$('entity-picker').addEventListener('click',event=>{if(event.target!==$('entity-picker'))return;const r=event.target.getBoundingClientRect();if(event.clientX<r.left||event.clientX>r.right||event.clientY<r.top||event.clientY>r.bottom)closePicker()});
function render(){
 const icon=slots[active].icon;$('icon-hint').textContent=!slots[active].entity?'':icon?'Home Assistant icon: '+icon:'Using the entity default. Sign in and refresh the entity list to sync Home Assistant icons.';
 visibleSlots=Math.max(visibleSlots,active+1,slots.reduce((last,s,i)=>s.entity?i+1:last,1));
 $('add-slot').disabled=busy||!initialized||visibleSlots>=MAX_SLOTS;
 $('slot-capacity').textContent=slots.filter(s=>s.entity).length+' configured · up to '+MAX_SLOTS;
 $('slots').replaceChildren(...slots.slice(0,visibleSlots).map((s,i)=>{const b=element('button','');b.append(element('b',String(i+1)),element('span',s.name||s.entity||'Empty slot'));if(dirtySlot(i))b.append(element('span','●','dirty-dot'));b.setAttribute('aria-label','Slot '+(i+1)+': '+(s.name||s.entity||'Empty')+(dirtySlot(i)?', unsaved':''));b.setAttribute('aria-current',String(i===active));b.disabled=busy;b.onclick=()=>{active=i;slotTab='display';playlistTarget=-1;render();search();refreshPreview()};return b}));
 const s=slots[active],restricted=nativePress(s.entity),count=slots.filter((_,i)=>dirtySlot(i)).length;
 $('slot-title').textContent='Slot '+(active+1);$('unsaved-count').textContent=count?count+' unsaved slot'+(count===1?'':'s'):'';
 $('action-binding').querySelector('h3').textContent=s.entity.startsWith('media_player.')?'When idle':'When pressed';
 renderEditorTabs();renderPlaylists();renderGestures();
 $('timer-options').hidden=!(s.entity.startsWith('timer.')||s.entity.startsWith('sensor.'));$('timer-hidden').checked=s.timerHidden==='1';
 $('sensor-options').hidden=!s.entity.startsWith('sensor.');$('secondary-entity').value=s.secondary;$('secondary-text').value=s.secondaryText;$('sensor-presentation').value=s.sensorGraph;
 $('entity-name').textContent=s.name||s.entity||'Empty slot';$('entity-id').textContent=s.entity;$('display-name').value=s.name;
 $('action-name').textContent=s.actionName||actionLabel(s.action);$('action-id').textContent=s.action&&s.action!=='none'?s.action:s.action==='none'?'Ignore a short press':'Use the entity’s normal controls';
 if(s.entity.startsWith('media_player.'))$('action-id').textContent=s.action&&s.action!=='none'?s.action:s.action==='none'?'Do nothing when idle':'Choose an automation or script to run when nothing is playing';
 if(restricted&&s.action)$('action-id').textContent='This saved action is not used by the device. Clear it with Default.';
 $('action-limit').hidden=!restricted;$('choose-action').disabled=busy||restricted||!s.entity;$('none').disabled=busy||restricted||!s.entity;$('default').disabled=busy||!s.entity;
 $('save').disabled=busy||!initialized||!dirtySlot(active);$('save-all').disabled=busy||!initialized||!count;$('discard').disabled=busy||!count;
 $('move-up').disabled=busy||active===0;$('move-down').disabled=busy||active===MAX_SLOTS-1;$('duplicate').disabled=busy||!s.entity||!slots.some(s=>!s.entity);
 for(const id of ['timer-hidden','choose-secondary','secondary-entity','secondary-text','sensor-presentation','choose-entity','clear','display-name','import','export','ha-connect','ha-disconnect','discard-settings'])$(id).disabled=busy||!initialized;
 $('save-settings').disabled=busy||!initialized||!dirtySettings();
 $('display-binding').classList.toggle('active',slotTab==='display');$('action-binding').classList.toggle('active',slotTab==='action');$('action-hint').hidden=!isActionPicker();$('search-title').textContent=isActionPicker()?'Automations & scripts':mode==='secondary'?'Secondary entities':'Available entities';$('picker-title').textContent=mode==='gesture'?'Choose a gesture action':mode==='gestureFeedback'?'Choose a feedback entity':mode==='playlist'?'Choose a playlist action':mode==='action'?'Choose a press action':mode==='secondary'?'Choose a secondary entity':'Choose a display entity';$('picker-context').textContent=mode==='playlist'?(slots[active].playlists[playlistTarget]?.name||'Playlist '+(playlistTarget+1)):slots[active].name||'Slot '+(active+1);
 $('filters').replaceChildren(...(isActionPicker()?[['actions','All actions'],['automation','Automations'],['script','Scripts']]:domains).map(([value,title])=>{const b=element('button',title);b.setAttribute('aria-pressed',String(value===domain));b.onclick=()=>{domain=value;render();search()};return b}));preview();
}
function changed(){if(recovery){recovery=null;$('draft-notice').hidden=true}persistDraft();render();message('Changes not saved. Review and save when ready.')}
function selectMode(value,index=-1){
 if(busy||!initialized)return;mode=value;playlistTarget=value==='playlist'?index:-1;domain=isActionPicker()?'actions':'';
 $('search').value='';$('room').value='';render();search();
 if(!$('entity-picker').open)$('entity-picker').showModal();$('search').focus();
}
function choose(item){
 if(busy)return;const s=slots[active];
 if(isActionPicker()&&!/^(automation|script)\./.test(item.entity))return;
 const target=playlistTarget,chosenMode=mode;
 if(mode==='gesture')s.gestures[gestureTarget]=item.entity;
 else if(mode==='gestureFeedback')s.gestures.feedback=item.entity;
 else if(mode==='playlist'){
  if(target<0||target>s.playlists.length||target>=8)return;
  if(target===s.playlists.length)s.playlists.push({name:'',action:''});
  const entry=s.playlists[target],old=ha.items.find(i=>i.entity===entry.action);
  if(!entry.name.trim()||entry.name===shortPlaylistName(old?.name||entry.action))entry.name=shortPlaylistName(item.name||item.entity);
  entry.action=item.entity;slotTab='playlists';
 }else if(mode==='secondary')s.secondary=item.entity;
 else if(mode==='entity'){
  if(s.entity!==item.entity){s.action='';s.actionName='';s.secondary='';s.secondaryText='{value}';s.sensorGraph='0';s.timerHidden='0';s.gestures=emptyGestures();s.playlists=[];s.typeOption=''}
  Object.assign(s,{entity:item.entity,name:item.name||item.entity,area:item.area||'',icon:item.icon||'',iconOverride:item.iconOverride||'0'});slotTab='display';
 }else{if(!s.entity||nativePress(s.entity))return;s.action=item.entity;s.actionName=item.name||actionLabel(item.entity)}
 closePicker();mode='entity';playlistTarget=-1;changed();search();
 if(chosenMode==='playlist')document.querySelector('[data-choose-playlist="'+target+'"]')?.focus();
 else if(chosenMode==='gesture'||chosenMode==='gestureFeedback')document.querySelector('[data-gesture-choose="'+(chosenMode==='gestureFeedback'?'feedback':gestureTarget)+'"]')?.focus();
 else $(chosenMode==='action'?'choose-action':chosenMode==='secondary'?'choose-secondary':'choose-entity').focus();
}
function isManualEntity(value){return /^[a-z0-9_]+\.[a-z0-9_]+$/.test(value)&&value.length<=128&&/^(sensor|binary_sensor|light|switch|climate|media_player|timer|weather|cover|automation|script|fan|number|input_number|select|input_select|scene|button|input_button)\./.test(value)&&mode!=='playlist'&&(!isActionPicker()||/^(automation|script)\./.test(value))}
function search(append=false){const mine=++revision,offset=append?next:0,query=$('search').value.trim();if(!append)$('results').replaceChildren();$('manual').replaceChildren();$('more').hidden=true;
 const available=isActionPicker()?ha.items.filter(i=>/^(automation|script)\./.test(i.entity)):ha.items;const items=$('room').value?available.filter(i=>i.area===$('room').value):available,data=OnxHA.search(items,query,domain,offset);if(mine!==revision)return;$('count').textContent=data.total+' matches';
 if(!data.items.length&&!append){$('results').append(element('p',ha.ready?'No matches. Try another name, room, or entity type.':mode==='playlist'?'Sign in on the Home Assistant page to choose a script or automation.':'Sign in on the Home Assistant page to browse entities, or enter a full entity ID.','empty'));if(!ha.ready){const b=element('button','Open Home Assistant setup');b.onclick=()=>{closePicker();page('connection')};$('results').append(b)}}
 for(const item of data.items){const row=element('button','','result'),text=element('span','');text.append(element('strong',item.name||item.entity),element('small',item.entity));row.append(text,element('em',item.area||item.entity.split('.')[0]));row.classList.toggle('selected',selectedPickerEntity()===item.entity);row.disabled=busy||(mode==='action'&&(!slots[active].entity||nativePress(slots[active].entity)));row.onclick=()=>choose(item);$('results').append(row)}
 next=data.next;$('more').hidden=next<0;if(isManualEntity(query)&&!ha.items.some(i=>i.entity===query)){const b=element('button','Use entity ID: '+query);b.disabled=busy;b.onclick=()=>choose({entity:query,name:query});$('manual').append(b)}
}
function weatherOptions(){const value=settings.weather||'',items=ha.items.filter(i=>i.entity.startsWith('weather.'));if(value&&!items.some(i=>i.entity===value))items.push({entity:value,name:value});$('standby-weather').replaceChildren(...[{entity:'',name:'None'},...items].map(item=>{const o=element('option',item.name||item.entity);o.value=item.entity;return o}));$('standby-weather').value=value}
function colorPicker(){const color=settings.media_color||'#62B0FF';$('media-color').value=color;$('color-value').textContent=color;$('ring-sample').style.setProperty('--ring-color',color);document.querySelectorAll('[data-ring-color]').forEach(button=>button.setAttribute('aria-pressed',String(button.dataset.ringColor===color)))}
function settingsForm(){colorPicker();$('date-format').value=settings.date_format||'dd-mm';$('hour-format').value=settings.hour_format||'24';$('show-ampm').checked=settings.show_ampm!=='0';$('show-ampm').disabled=settings.hour_format!=='12';$('standby-scope').value=settings.standby_scope||'main';$('idle-view').value=settings.idle_view||'clock';for(const key of ['brightness','standby','sensitivity'])$(key).value=settings[key];$('sleep-enabled').checked=settings.sleep_enabled==='1';$('sleep-start').value=settings.sleep_start;$('sleep-end').value=settings.sleep_end;$('sleep-timeout').value=settings.sleep_timeout;weatherOptions()}
function roomOptions(){const value=$('room').value;$('room').replaceChildren(...['',...new Set(ha.items.map(i=>i.area).filter(Boolean).sort())].map(room=>{const o=element('option',room||'All rooms');o.value=room;return o}));$('room').value=value;if(!$('room').value)$('room').value=''}
function validatePlaylists(entries,entity){
 if(!Array.isArray(entries)||entries.length>8||entries.length&&!entity.startsWith('media_player.'))throw Error('Playlists require a media player and a maximum of eight entries.');
 return entries.map(entry=>{if(!entry||typeof entry.name!=='string'||!entry.name.trim()||/[\r\n]/.test(entry.name)||new TextEncoder().encode(entry.name).length>48||typeof entry.action!=='string'||entry.action.length>128||!/^(automation|script)\.[a-z0-9_]+$/.test(entry.action))throw Error('Each playlist needs a name (up to 48 UTF-8 bytes) and a script or automation entity ID.');return {name:entry.name,action:entry.action}});
}
function validateGestures(input){
 const value={...emptyGestures(),...(input||{})},result={};
 for(const key of ['cw','ccw','holdCw','holdCcw']){const action=value[key];if(typeof action!=='string'||action.length>128||action&&!/^(none|(?:automation|script)\.[a-z0-9_]+)$/.test(action))throw Error('Choose a script, automation, Default or No action for each gesture.');result[key]=action}
 if(typeof value.feedback!=='string'||value.feedback.length>128||value.feedback&&!/^[a-z_]+\.[a-z0-9_]+$/.test(value.feedback))throw Error('Choose a valid feedback entity.');result.feedback=value.feedback;
 for(const key of ['popup','responses','events']){if(!['0','1'].includes(value[key]))throw Error('Invalid gesture feedback option.');result[key]=value[key]}
 if(typeof value.step!=='string'||!/^\d{1,3}$/.test(value.step)||+value.step<1||+value.step>100)throw Error('Gesture step must be 1–100.');result.step=value.step;return result;
}
function renderGestures(){
 const gesture=slots[active].gestures;
 for(const key of ['cw','ccw','holdCw','holdCcw']){
  $('gesture-'+key).value=gesture[key];$('gesture-'+key).disabled=busy||!initialized||!slots[active].entity;
 }
 $('gesture-feedback').value=gesture.feedback;$('gesture-step').value=gesture.step;
 for(const key of ['popup','responses','events'])$('gesture-'+key).checked=gesture[key]==='1';
 for(const node of document.querySelectorAll('[data-gesture-choose],[data-gesture-reset],#gesture-feedback,#gesture-step,#gesture-popup,#gesture-responses,#gesture-events'))node.disabled=busy||!initialized||!slots[active].entity;
}
for(const key of ['cw','ccw','holdCw','holdCcw']){
 $('gesture-'+key).onchange=()=>{slots[active].gestures[key]=$('gesture-'+key).value.trim();changed()};
}
for(const button of document.querySelectorAll('[data-gesture-choose]'))button.onclick=()=>{gestureTarget=button.dataset.gestureChoose;selectMode(gestureTarget==='feedback'?'gestureFeedback':'gesture')};
for(const button of document.querySelectorAll('[data-gesture-reset]'))button.onclick=()=>{slots[active].gestures[button.dataset.gestureReset]='';changed()};
$('gesture-feedback').onchange=()=>{slots[active].gestures.feedback=$('gesture-feedback').value.trim();changed()};
$('gesture-step').onchange=()=>{slots[active].gestures.step=$('gesture-step').value;changed()};
for(const key of ['popup','responses','events'])$('gesture-'+key).onchange=()=>{slots[active].gestures[key]=$('gesture-'+key).checked?'1':'0';changed()};
function validateSlots(input){if(!Array.isArray(input)||![10,MAX_SLOTS].includes(input.length))throw Error('A configuration must contain ten or twenty slots.');input=[...input,...Array.from({length:MAX_SLOTS-input.length},empty)];return input.map(s=>{if(!s||typeof s!=='object')throw Error('Invalid slot in configuration.');const result=empty();for(const key of Object.keys(result)){if(key==='playlists'||key==='gestures')continue;const value=s[key]??(['iconOverride','sensorGraph','timerHidden'].includes(key)?'0':key==='secondaryText'?'{value}':'');if(typeof value!=='string'||value.length>(['entity','action','secondary'].includes(key)?128:key==='secondaryText'?160:key==='typeOption'?32:192)||new TextEncoder().encode(value).length>(['entity','action','secondary'].includes(key)?128:key==='secondaryText'?160:key==='typeOption'?32:192))throw Error('Invalid or oversized slot field: '+key);result[key]=value}if(!['0','1'].includes(result.timerHidden))throw Error('Invalid timer visibility.');result.gestures=validateGestures(s.gestures);result.playlists=validatePlaylists(s.playlists??[],result.entity);if(result.secondary&&!/^[a-z_]+\.[a-z0-9_]+$/.test(result.secondary)||!['0','1'].includes(result.sensorGraph))throw Error('Choose a valid secondary entity and sensor display.');if(result.icon&&!/^mdi:[a-z0-9-]{1,76}$/.test(result.icon)||!['0','1'].includes(result.iconOverride))throw Error('Invalid entity icon.');if(result.entity&&!isSupported(result.entity))throw Error('Unsupported entity: '+result.entity);if(result.action&&!/^(none|(?:automation|script)\.[a-z0-9_]+)$/.test(result.action))throw Error('Unsupported press action.');if(!result.entity&&result.action)throw Error('An empty slot cannot have a press action.');if(result.typeOption&&!choices(result).includes(result.typeOption))throw Error('Display type does not match its entity.');return result})}
function isSupported(value){return /^\[Local\] HMI IO(10|11|17|18)$/.test(value)||/^(sensor|binary_sensor|light|switch|climate|media_player|timer|weather|cover|automation|script|fan|number|input_number|select|input_select|scene|button|input_button)\.[a-z0-9_]+$/.test(value)}
function validateSettings(value){if(!value||typeof value!=='object')throw Error('Device settings are missing.');value={idle_view:'clock',media_color:'#62B0FF',standby_scope:'main',date_format:'dd-mm',hour_format:'24',show_ampm:'1',...value};const result={};for(const key of ['idle_view','show_ampm','date_format','hour_format','media_color','standby_scope','brightness','standby','sensitivity','sleep_enabled','sleep_start','sleep_end','sleep_timeout','weather']){if(typeof value[key]!=='string')throw Error('Invalid device setting: '+key);result[key]=value[key]}if(!['clock','media'].includes(result.idle_view))throw Error('Choose a valid idle screen.');if(!/^#[0-9a-f]{6}$/i.test(result.media_color)||!['main','all','never'].includes(result.standby_scope))throw Error('Choose a valid progress color and weather timeout behavior.');if(!['0','1'].includes(result.show_ampm)||!['dd-mm','mm-dd','yyyy-mm-dd','dd-mmm','mmm-dd','yyyy-mmm-dd'].includes(result.date_format)||!['12','24'].includes(result.hour_format))throw Error('Choose a valid date and clock format.');result.media_color=result.media_color.toUpperCase();for(const [key,min,max] of [['brightness',10,100],['standby',1,900],['sleep_timeout',1,3600]])if(!/^\d+$/.test(result[key])||+result[key]<min||+result[key]>max)throw Error('Invalid value for '+key);if(!['1','2','4'].includes(result.sensitivity)||!['0','1'].includes(result.sleep_enabled)||!/^([01]\d|2[0-3]):[0-5]\d$/.test(result.sleep_start)||!/^([01]\d|2[0-3]):[0-5]\d$/.test(result.sleep_end)||result.weather&&!/^weather\.[a-z0-9_]+$/.test(result.weather))throw Error('Invalid sleep, sensitivity, or weather setting.');return result}
function validateDocument(value){if(![1,2].includes(value?.schema))throw Error('Unsupported configuration version.');return {slots:validateSlots(value.slots),settings:validateSettings(value.settings)}}
async function saveSlots(indices){if(busy||!initialized)return;busy=true;render();search();let completed=0;try{for(const i of indices){const snapshot=copy(slots[i]),item=ha.items.find(item=>item.entity===snapshot.entity);if(item&&Object.hasOwn(item,'icon')){snapshot.icon=item.icon||'';snapshot.iconOverride=item.iconOverride||'0';slots[i].icon=snapshot.icon;slots[i].iconOverride=snapshot.iconOverride;}validateSlots([snapshot,...Array.from({length:MAX_SLOTS-1},empty)]);if(nativePress(snapshot.entity)&&snapshot.action)throw Error('Choose Default for the press action on slot '+(i+1)+'.');if(item?.types&&snapshot.typeOption&&!item.types.includes(snapshot.typeOption))throw Error('Slot '+(i+1)+' uses an unsupported control type. Choose a type detected by Home Assistant.');message('Saving slot '+(i+1)+'…');await rpc(slotPayload(i,snapshot));saved[i]=snapshot;completed++;persistDraft()}message(completed+' slot'+(completed===1?'':'s')+' saved.')}catch(error){message(error.message+' '+completed+' slots saved; remaining changes are still marked unsaved.',true)}finally{busy=false;persistDraft();render();search()}}
$('save').onclick=()=>saveSlots([active]);$('save-all').onclick=()=>saveSlots(slots.map((_,i)=>i).filter(dirtySlot));
$('discard').onclick=()=>{if(!confirm('Discard all unsaved slot changes?'))return;slots=copy(saved);changed();search();message('Unsaved slot changes discarded.')};
$('choose-secondary').onclick=()=>selectMode('secondary');
$('choose-entity').onclick=()=>selectMode('entity');$('choose-action').onclick=()=>selectMode('action');$('default').onclick=()=>{slots[active].action='';slots[active].actionName='';changed()};$('none').onclick=()=>{slots[active].action='none';slots[active].actionName='';changed()};$('clear').onclick=()=>{slots[active]=empty();changed();search()};
$('display-name').oninput=()=>{slots[active].name=$('display-name').value;persistDraft();$('save').disabled=busy||!dirtySlot(active);$('save-all').disabled=busy||!slots.some((_,i)=>dirtySlot(i));$('discard').disabled=!slots.some((_,i)=>dirtySlot(i));$('unsaved-count').textContent='Unsaved changes';preview();message('Display name not saved.')};$('display-name').onchange=()=>render();
$('timer-hidden').onchange=()=>{slots[active].timerHidden=$('timer-hidden').checked?'1':'0';changed()};
for(const field of ['secondary-entity','secondary-text','sensor-presentation'])$(field).oninput=()=>{slots[active].secondary=$('secondary-entity').value.trim();slots[active].secondaryText=$('secondary-text').value;slots[active].sensorGraph=$('sensor-presentation').value;changed()};
function move(delta){if(busy)return;const target=active+delta;[slots[active],slots[target]]=[slots[target],slots[active]];active=target;changed();search()}
$('add-slot').onclick=()=>{if(busy||visibleSlots>=MAX_SLOTS)return;active=visibleSlots++;slotTab='display';render();search();refreshPreview()};
$('move-up').onclick=()=>move(-1);$('move-down').onclick=()=>move(1);$('duplicate').onclick=()=>{const target=slots.findIndex(s=>!s.entity);if(target<0||busy)return;slots[target]=copy(slots[active]);active=target;changed();search()};
$('search').oninput=()=>{clearTimeout(debounce);debounce=setTimeout(()=>search(),200)};$('room').onchange=()=>search();$('more').onclick=()=>search(true);
$('preview-screen').onchange=()=>drawPreview(slots[active]);
document.querySelectorAll('[data-ring-color]').forEach(button=>button.onclick=()=>{$('media-color').value=button.dataset.ringColor;$('settings-form').dispatchEvent(new Event('input',{bubbles:true}))});
$('settings-form').oninput=()=>{if(!initialized)return;settings={idle_view:$('idle-view').value,show_ampm:$('show-ampm').checked?'1':'0',date_format:$('date-format').value,hour_format:$('hour-format').value,media_color:$('media-color').value.toUpperCase(),standby_scope:$('standby-scope').value,brightness:$('brightness').value,standby:$('standby').value,sensitivity:$('sensitivity').value,sleep_enabled:$('sleep-enabled').checked?'1':'0',sleep_start:$('sleep-start').value,sleep_end:$('sleep-end').value,sleep_timeout:$('sleep-timeout').value,weather:$('standby-weather').value};$('show-ampm').disabled=settings.hour_format!=='12';colorPicker();drawPreview(slots[active]);persistDraft();$('save-settings').disabled=busy||!dirtySettings();message('Device settings not saved.')};
$('discard-settings').onclick=()=>{settings=copy(savedSettings);settingsForm();persistDraft();render();message('Unsaved device settings discarded.')};
$('settings-form').onsubmit=async event=>{event.preventDefault();if(busy||!initialized)return;try{validateSettings(settings)}catch(error){message(error.message,true);return}busy=true;render();const snapshot=copy(settings);try{message('Saving device settings…');await rpc({op:'settings',...snapshot});savedSettings={...snapshot,weather:savedWeather};persistDraft();if(snapshot.weather!==savedWeather){const item=ha.items.find(i=>i.entity===snapshot.weather)||{};await rpc({op:'weather',entity:snapshot.weather,name:item.name||snapshot.weather,area:item.area||''});savedWeather=snapshot.weather}savedSettings=snapshot;message('Device settings saved.')}catch(error){message(error.message+' Review the settings and retry any unsaved changes.',true)}finally{busy=false;persistDraft();render()}};
async function syncIconMetadata(){
 let count=0;
 for(let i=0;i<saved.length;i++){
  const source=saved[i],item=ha.items.find(item=>item.entity===source.entity);
  if(!source.entity||!item||!Object.hasOwn(item,'icon'))continue;
  const icon=item.icon||'',iconOverride=item.iconOverride||'0';
  if(source.icon===icon&&source.iconOverride===iconOverride)continue;
  await rpc({op:'icon',slot:i,entity:source.entity,icon,icon_override:iconOverride});
  source.icon=icon;source.iconOverride=iconOverride;
  if(slots[i].entity===source.entity){slots[i].icon=icon;slots[i].iconOverride=iconOverride}
  count++;
 }
 return count;
}
async function refreshCatalogue(){
 if(busy)return;$('refresh').disabled=true;message('Loading entities from Home Assistant…');
 try{
  await ha.load();lastLoaded=new Date();
  const icons=await syncIconMetadata();persistDraft();weatherOptions();roomOptions();render();search();
  message('Entity list refreshed.'+(icons?' '+icons+' device icon'+(icons===1?'':'s')+' synced.':'')+' Unsaved slot edits are preserved.');
 }catch(error){message(error.message+' Check the address and sign in again if needed.',true)}
 finally{$('refresh').disabled=false;browserStatus()}
}
$('refresh').onclick=refreshCatalogue;$('ha-connect').onclick=async()=>{persistDraft();try{const origin=OnxHA.baseUrl($('ha-url').value);await rpc({op:'ha_host',q:origin});await ha.login(origin)}catch(error){message(error.message,true)}};$('ha-disconnect').onclick=()=>{ha.disconnect();lastLoaded=null;roomOptions();weatherOptions();browserStatus();render();search();message('Browser disconnected. The knob’s Home Assistant connection is unchanged.')};
async function refreshStatus(){if(statusBusy||busy||!initialized)return;statusBusy=true;try{device=await rpc({op:'state'});deviceStatus(true)}catch(error){deviceStatus(false);message(error.message,true)}finally{statusBusy=false}}
$('check-status').onclick=refreshStatus;
$('retry-states').onclick=async()=>{if(busy)return;$('retry-states').disabled=true;try{await rpc({op:'retry'});message('Entity state refresh queued. Waiting for Home Assistant…');await refreshStatus()}catch(error){message(error.message,true)}finally{$('retry-states').disabled=false}};
function download(name,value){const url=URL.createObjectURL(new Blob([JSON.stringify(value,null,2)],{type:'application/json'})),a=element('a','');a.href=url;a.download=name;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000)}
$('export').onclick=()=>download('onx2424g013-config.json',{schema:2,model:'ONX2424G013',exported_at:new Date().toISOString(),slots:saved,settings:savedSettings});
$('diagnostics').onclick=async()=>{try{const icons=await rpc({op:'icon_status'});download('onx2424g013-diagnostics.json',{model:'ONX2424G013',firmware:device.firmware||'unknown',uptime_ms:device.uptime_ms,reset_reason:device.reset_reason,reset_reason_text:device.reset_reason_text,internal_heap_free:device.internal_heap_free,internal_heap_minimum:device.internal_heap_minimum,internal_heap_largest_block:device.internal_heap_largest_block,psram_free:device.psram_free,artwork_status:device.artwork_status,artwork_error:device.artwork_error,wifi_rssi_dbm:device.wifi_rssi_dbm,wifi_disconnects_since_boot:device.wifi_disconnects_since_boot,last_wifi_disconnect_ms:device.last_wifi_disconnect_ms,last_event_before_boot:device.last_event_before_boot,wifi_connected:device.wifi_connected,ha_connected:device.ha_connected,browser_catalogue_ready:ha.ready,captured_at:new Date().toISOString(),icons:icons.slots||[]});}catch(error){message('Could not load icon diagnostics: '+error.message,true)}};
$('import').onclick=()=>$('import-file').click();$('import-file').onchange=async()=>{const file=$('import-file').files[0];try{if(!file)return;if(file.size>65536)throw Error('Configuration file is too large (maximum 64 KB).');const value=JSON.parse(await file.text());if(value.model!=='ONX2424G013')throw Error('This configuration belongs to a different device model.');const draft=validateDocument(value);if(hasChanges()&&!confirm('Replace your unsaved draft with this imported configuration?'))return;slots=draft.slots;settings=draft.settings;settingsForm();changed();search();page('entities');message('Configuration imported as a draft. Review and save the slots and device settings to apply.')}catch(error){message('Import failed: '+error.message,true)}finally{$('import-file').value=''}};
$('restore-draft').onclick=()=>{if(!recovery)return;slots=recovery.slots;settings=recovery.settings;recovery=null;$('draft-notice').hidden=true;settingsForm();changed();search();message('Draft restored. Review and save to apply it to the knob.')};$('dismiss-draft').onclick=()=>{recovery=null;$('draft-notice').hidden=true;try{sessionStorage.removeItem(DRAFT)}catch{}};
window.addEventListener('beforeunload',event=>{if(hasChanges()||busy){event.preventDefault();event.returnValue=''}});
try{const theme=localStorage.getItem('onx.theme')||'system';$('theme').value=['light','dark','system'].includes(theme)?theme:'system';if($('theme').value!=='system')document.documentElement.dataset.theme=$('theme').value}catch{}
$('theme').onchange=()=>{const theme=$('theme').value;if(theme==='system')delete document.documentElement.dataset.theme;else document.documentElement.dataset.theme=theme;try{localStorage.setItem('onx.theme',theme)}catch{}};
(async()=>{render();search();try{
 device=await rpc({op:'state'});slots=validateSlots(device.slots);saved=copy(slots);savedWeather=device.weather||'';const data=await rpc({op:'settings_state'});settings=validateSettings({...data,weather:savedWeather});savedSettings=copy(settings);initialized=true;deviceStatus(true);$('ha-url').value=device.ha_url||'';settingsForm();
 const draft=safeRead(DRAFT);if(draft){try{recovery=validateDocument(draft);$('draft-notice').hidden=false}catch{message('An older recovery draft could not be read. Your saved configuration is unchanged.',true)}}
 await ha.finishLogin();const auth=ha.read('onx.ha.auth');if(auth)$('ha-url').value=auth.base;browserStatus();render();search();if(auth)await refreshCatalogue();else message('Knob settings loaded. Sign in to Home Assistant to browse entities.');await refreshPreview();
 }catch(error){if(!initialized)deviceStatus(false);message(error.message,true);render();browserStatus()}
 document.addEventListener('visibilitychange',()=>{if(!document.hidden){refreshStatus();refreshPreview()}});setInterval(()=>{if(slots[active].entity.startsWith('media_player.')||Date.now()-lastPreviewAt>=2000)refreshPreview()},500);setInterval(()=>{if(!document.hidden)refreshStatus()},30000);
})();
})();
)ONXJS";
}
