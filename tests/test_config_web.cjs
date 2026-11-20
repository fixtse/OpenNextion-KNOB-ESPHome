const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { chromium } = require('playwright');

(async () => {
 const browser = await chromium.launch({headless:true, args:['--no-sandbox']});
 try {
  const page = await browser.newPage({viewport:{width:1100,height:850}});
  const html = fs.readFileSync('my_components/onx_config_web/config_page.h','utf8').split('R"ONXHTML(')[1].split(')ONXHTML";')[0];
  const entities = [
   {entity:'sensor.living_room_temperature',name:'Living room temperature',area:'Living room'},
   {entity:'fan.office',name:'Office fan',area:'Office',icon:'mdi:ceiling-fan',iconOverride:'1'},
   {entity:'select.office_mode',name:'Office mode',area:'Office'},
   {entity:'binary_sensor.front_door',name:'Front door',area:'Hallway'},
   {entity:'script.comfort',name:'Comfort scene',area:'Living room'},
   {entity:'automation.welcome',name:'Welcome home',area:'Hallway'}
  ];
  const slots = Array.from({length:10},()=>({entity:'',name:'',action:''}));
  slots[9]={entity:'fan.office',name:'Office fan',action:'',icon:'',iconOverride:'0'};
  slots[7]={entity:'climate.calentador',name:'Calentador',action:''};
  slots[8]={entity:'media_player.bedroom',name:'Bedroom',action:''};
  const results = new Map(); let ticket=0, failSave=false, saved=[],iconUpdates=[],savedSettings=[];
  const presetStore={presets:[],active:'',applying:'',capacity:8,remaining:0},presetOps=[];
  await page.route('http://knob.test/**',async route=>{
   const request=route.request(),url=new URL(request.url());
   if(url.pathname==='/'||url.pathname==='/knob'){await route.fulfill({contentType:'text/html',body:html});return}
   if(url.pathname==='/knob/ha.js'){
    const source=fs.readFileSync('my_components/onx_config_web/ha_browser.h','utf8').split('R"ONXJS(')[1].split(')ONXJS"')[0];
    const mock=`OnxHA.Client=class {constructor(){this.items=${JSON.stringify(entities)};this.ready=true}read(){return {base:'http://ha.test:8123'}}async finishLogin(){}async load(){return this.items}disconnect(){this.items=[];this.ready=false}};`;
    await route.fulfill({contentType:'text/javascript',body:source+'\n'+mock});return;
   }
   if(url.pathname==='/knob/app.js'){
    const source=fs.readFileSync('my_components/onx_config_web/config_app.h','utf8').split('R"ONXJS(')[1].split(')ONXJS"')[0];
    await route.fulfill({contentType:'text/javascript',body:source});return;
   }
   if(url.pathname==='/knob/fixtse-logo.png'){
    await route.fulfill({contentType:'image/png',body:Buffer.from('iVBORw0KGgo=', 'base64')});return;
   }
   if(url.pathname==='/knob/api'){
    const values=new URLSearchParams(request.postData());
    let result;
    if(values.get('op')==='state')result={count:0,slots,weather:'',ha_url:'http://ha.test:8123'};
    else if(values.get('op')==='preview')result={slots:slots.filter(s=>s.entity&&s.entity===values.get('entity')).map(s=>{
     const media=s.entity.startsWith('media_player.'),climate=s.entity.startsWith('climate.');
     const detail={entity:s.entity,layout:media?'media':'value',available:true,icon:'mdi:thermometer',icon_x:media||!climate?0:-62,icon_y:media?18:climate?32:134,icon_size:media?80:climate?28:40,progress:media?470:-1,playing:media,artwork:media?'http://ha.test/artwork.svg':'',lines:[
      {text:media?'':climate?'🔥 Heat':'🌡 Temperature',y:36,size:16,width:156,x:climate?16:0,color:climate?0xffaa66:0x62b0ff},
      {text:media?'Volume: 50%':climate?'21.5°':'16.63 °C',y:media?166:climate?75:84,size:media?18:32,width:208},
      {text:media?'Silent Night / Noche de Paz':climate?'Now 23°':'',y:media?122:119,size:16,width:196},
      {text:media?'Christina Aguilera':climate?'Press: mode\nHold + turn: setting':'',y:media?144:147,size:media?16:14,width:198},
      {text:s.name||s.entity,y:media?192:185,size:16,width:180}
     ]};
     detail.carousel={layout:'carousel',available:true,icon:'mdi:thermometer',icon_x:0,icon_y:34,icon_size:40,pagination:'· · · · ● · ·',navigator:[{visible:true,selected:true,icon:'mdi:thermometer',offset:0}],lines:[
      {text:'',y:36,size:16,width:156},
      {text:media?'Playing':climate?'21.5°':'16.63 °C',y:88,size:32,width:216},
      {text:media?'Silent Night':climate?'🔥 Heat':'',y:124,size:14,width:198},
      {text:'',y:147,size:14,width:198},
      {text:s.name||s.entity,y:145,size:16,width:184}]};
     return detail;
    })};
    else if(values.get('op')==='icon'){iconUpdates.push(Object.fromEntries(values));Object.assign(slots[Number(values.get('slot'))],{icon:values.get('icon'),iconOverride:values.get('icon_override')});result={ok:true}}
    else if(values.get('op')==='settings'){savedSettings.push(Object.fromEntries(values));result={ok:true}}
    else if(values.get('op')==='settings_state')result={brightness:'100',standby:'300',sensitivity:'2',sleep_enabled:'0',sleep_start:'23:00',sleep_end:'07:00',sleep_timeout:'300'};
    else if(values.get('op')==='save'){
     if(failSave)result={error:'Could not save the press action; retry'};
     else {saved.push(Object.fromEntries(values));slots[Number(values.get('slot'))]={entity:values.get('entity'),action:values.get('action'),playlists:JSON.parse(values.get('playlists')||'[]')};result={ok:true}}
    }else if(values.get('op')?.startsWith('preset')){
     const op=values.get('op'),name=values.get('name');presetOps.push(Object.fromEntries(values));
     const found=presetStore.presets.find(p=>p.name===name);
     if(op==='preset_save'){if(found)found.controls=slots.filter(s=>s.entity).length;else presetStore.presets.push({name,controls:slots.filter(s=>s.entity).length,bytes:900});presetStore.active=name}
     else if(op==='preset_apply'){if(!found)result={error:'Preset not found'};else presetStore.active=name}
     else if(op==='preset_rename'){found.name=values.get('q');if(presetStore.active===name)presetStore.active=found.name}
     else if(op==='preset_get'){if(!found)result={error:'Preset not found'};else{const text=JSON.stringify({name,slots:presetStore.docs?.[name]||Array.from({length:20},(_,i)=>({entity:i===0?'sensor.living_room_temperature':'',name:i===0?'Temp':''}))}),offset=Number(values.get('offset')||0);const bytes=Buffer.from(text),next=Math.min(bytes.length,offset+200);result={name,total:bytes.length,offset,next,data:bytes.subarray(offset,next).toString()};presetStore.chunks=(presetStore.chunks||0)+1}}
     else if(op==='preset_put'){const upload=presetStore.upload=values.get('offset')==='0'?'':presetStore.upload;assert.equal(Buffer.byteLength(upload),Number(values.get('offset')),'Upload offsets count UTF-8 bytes');presetStore.upload=upload+values.get('data');if(Buffer.byteLength(presetStore.upload)===Number(values.get('total'))){const doc=JSON.parse(presetStore.upload);assert.equal(doc.slots.length,20);if(!found)presetStore.presets.push({name,controls:doc.slots.filter(s=>s.entity).length,bytes:presetStore.upload.length});presetStore.docs=presetStore.docs||{};presetStore.docs[name]=doc.slots;presetStore.upload=''}else result={received:presetStore.upload.length}}
     else if(op==='preset_delete'){presetStore.presets=presetStore.presets.filter(p=>p.name!==name);if(presetStore.active===name)presetStore.active=''}
     result=result||JSON.parse(JSON.stringify(presetStore));
    }else if(values.get('op')==='refresh'||values.get('op')==='search')throw Error('Catalogue must stay in browser');else result={ok:true};
    results.set(++ticket,result);await route.fulfill({status:202,json:{ticket}});return;
   }
   if(url.pathname==='/knob/result'){await route.fulfill({json:results.get(Number(url.searchParams.get('ticket')))});return}
   throw Error('Unexpected request: '+request.url());
  });
  await page.route('http://ha.test/artwork.svg',route=>route.fulfill({contentType:'image/svg+xml',body:'<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"><rect width="100" height="100" fill="#b91c1c"/></svg>'}));
  await page.goto('http://knob.test/');
  await page.waitForFunction(()=>!document.getElementById('choose-entity').disabled);
  await page.getByRole('status').filter({hasText:'Entity list refreshed.'}).waitFor();
  if(await page.locator('#entity-picker').isVisible())await page.locator('#close-picker').click();
  await page.locator('#slot-tab-display').click();
  await page.locator('#choose-entity').click();
  assert.equal(iconUpdates.length,1);assert.equal(iconUpdates[0].slot,'9');assert.equal(iconUpdates[0].icon,'mdi:ceiling-fan');
  await page.getByRole('button',{name:'Living room temperature'}).waitFor();
  await page.locator('#search').fill('living room');
  await page.getByRole('button',{name:'Living room temperature'}).click();
  await page.locator('#slot-tab-action').click();
  await page.locator('#choose-action').click();
  await page.getByRole('button',{name:'Comfort scene'}).click();
  assert.equal(await page.locator('#entity-id').textContent(),'sensor.living_room_temperature');
  assert.equal(await page.locator('#action-name').textContent(),'Comfort scene');
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.equal(saved[0].action,'script.comfort');assert.equal(saved[0].entity,'sensor.living_room_temperature');
  await page.locator('#device-preview text').filter({hasText:'16.63 °C'}).waitFor();
  assert.equal(await page.locator('#device-preview text').filter({hasText:'16.63 °C'}).getAttribute('font-size'),'32');
  await page.locator('#preview-screen').selectOption('main');
  assert.match(await page.locator('#device-preview image').first().getAttribute('href'),/mdi\/thermometer/);
  await page.locator('#preview-screen').selectOption('detail');
  await page.getByRole('button',{name:'Slot 2: Empty',exact:true}).click();
  if(await page.locator('#entity-picker').isVisible())await page.locator('#close-picker').click();
  await page.locator('#slot-tab-display').click();
  await page.locator('#choose-entity').click();
  await page.getByRole('button',{name:'Front door'}).click();
  await page.locator('#slot-tab-action').click();
  await page.locator('#none').click();failSave=true;
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'Could not save the press action'}).waitFor();
  assert.equal(await page.locator('#save').isDisabled(),false);
  failSave=false;await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.equal(saved[1].action,'none');
  await page.getByRole('button',{name:'Slot 1: Living room temperature',exact:true}).click();
  assert.equal(await page.locator('#action-name').textContent(),'Comfort scene');
  await page.locator('#slot-tab-action').click();
  await page.locator('#default').click();
  assert.equal(await page.locator('#action-name').textContent(),'Device default');
  await page.locator('#slot-tab-action').click();
  await page.locator('#choose-action').click();
  await page.locator('#search').fill('script.manual');
  await page.getByRole('button',{name:'Use entity ID: script.manual'}).waitFor();
  if(await page.locator('#entity-picker').isVisible())await page.locator('#close-picker').click();
  await page.locator('#slot-tab-display').click();
  await page.locator('#choose-entity').click();
  await page.locator('#search').fill('Office fan');
  await page.locator('#results').getByRole('button',{name:'Office fan'}).click();
  assert.equal(await page.locator('#choose-action').isDisabled(),true);
  assert.match(await page.locator('#rotate-hint').textContent(),/speed/);
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.equal(saved.at(-1).icon,'mdi:ceiling-fan');assert.equal(saved.at(-1).icon_override,'1');
  if(await page.locator('#entity-picker').isVisible())await page.locator('#close-picker').click();
  await page.locator('#slot-tab-display').click();
  await page.locator('#choose-entity').click();
  await page.locator('#search').fill('Office mode');
  await page.getByRole('button',{name:'Office mode'}).click();
  assert.match(await page.locator('#press-hint').textContent(),/Confirm option/);
  await page.getByRole('button',{name:'Home Assistant',exact:true}).click();
  await page.locator('#retry-states').click();
  await page.getByRole('status').filter({hasText:'Entity state refresh queued'}).waitFor();
  await page.getByRole('button',{name:'Display & actions',exact:true}).click();
  const screenshots=path.dirname(process.env.ONX_UI_SCREENSHOT||'/tmp/onx-config-mobile.png');
  await page.getByRole('button',{name:'Slot 8: Calentador',exact:true}).click();
  await page.locator('#device-preview text').filter({hasText:'🔥 Heat'}).waitFor();
  assert.match(await page.locator('#device-preview').textContent(),/21.5°/);
  await page.screenshot({path:path.join(screenshots,'onx-climate-preview.png'),fullPage:true});
  await page.getByRole('button',{name:'Slot 9: Bedroom',exact:true}).click();
  await page.locator('#slot-tab-playlists').click();
  assert.equal(await page.locator('#playlist-options').isVisible(),true);
  assert.equal(await page.locator('#advanced-type').count(),0);
  await page.locator('#add-playlist').click();
  await page.locator('#results').getByRole('button',{name:'Comfort scene'}).click();
  assert.equal(await page.getByRole('textbox',{name:'Quick action 1 name',exact:true}).inputValue(),'Comfort scene');
  assert.equal(await page.getByRole('textbox',{name:'Quick action 1 action',exact:true}).count(),0);
  await page.getByRole('textbox',{name:'Quick action 1 name',exact:true}).fill('Jazz radio');
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.deepEqual(JSON.parse(saved.at(-1).playlists),[{name:'Jazz radio',action:'script.comfort'}]);
  await page.locator('#add-playlist').click();
  await page.locator('#close-picker').click();
  assert.equal(await page.locator('.playlist-row').count(),1,'Cancelling the picker must not add an empty entry');
  await page.locator('#add-playlist').click();
  await page.locator('#results').getByRole('button',{name:'Welcome home'}).click();
  await page.getByRole('button',{name:'Move up quick action 2',exact:true}).click();
  assert.equal(await page.getByRole('textbox',{name:'Quick action 1 name',exact:true}).inputValue(),'Welcome home');
  await page.getByRole('button',{name:'Remove quick action 1',exact:true}).click();
  assert.equal(await page.getByRole('textbox',{name:'Quick action 1 name',exact:true}).inputValue(),'Jazz radio');

  await page.locator('#device-preview text').filter({hasText:'Christina Aguilera'}).waitFor();
  assert.equal(await page.locator('.preview-ring').getAttribute('stroke-dasharray'),'470 530');
  assert.match(await page.locator('#device-preview').textContent(),/Volume: 50%/);
  assert.equal(await page.locator('#device-preview image').last().getAttribute('width'),'100');
  await page.screenshot({path:path.join(screenshots,'onx-media-preview.png'),fullPage:true});
  assert.equal(await page.locator('#choose-action').isDisabled(),false);
  assert.equal(await page.locator('#action-binding h3').textContent(),'When idle');
  for(const [name,entity] of [['Welcome home','automation.welcome'],['Comfort scene','script.comfort']]){
   await page.locator('#slot-tab-action').click();
  await page.locator('#choose-action').click();
   await page.locator('#results').getByRole('button',{name,exact:false}).click();
   await page.locator('#save').click();
   await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
   assert.equal(saved.at(-1).action,entity);
  }
  assert.match(await page.locator('#press-hint').textContent(),/Playing\/paused: play\/pause.*Comfort scene/);

  await page.getByRole('button',{name:'Device settings',exact:true}).click();
  assert.equal(await page.locator('#standby-scope').inputValue(),'main');
  await page.getByRole('button',{name:'Soft violet',exact:true}).click();
  assert.equal(await page.locator('#media-color').inputValue(),'#a78bfa');
  await page.screenshot({path:path.join(screenshots,'onx-color-settings.png'),fullPage:true});
  await page.locator('#standby-scope').selectOption('all');
  assert.equal(await page.locator('#date-format').inputValue(),'dd-mm');
  await page.locator('#date-format').selectOption('yyyy-mmm-dd');
  await page.locator('#hour-format').selectOption('12');
  await page.locator('#save-settings').click();
  await page.getByRole('status').filter({hasText:'Device settings saved.'}).waitFor();
  assert.equal(savedSettings.at(-1).media_color,'#A78BFA');assert.equal(savedSettings.at(-1).standby_scope,'all');
  assert.equal(savedSettings.at(-1).date_format,'yyyy-mmm-dd');assert.equal(savedSettings.at(-1).hour_format,'12');
  await page.getByRole('button',{name:'Display & actions',exact:true}).click();
  assert.equal(await page.locator('.preview-ring').getAttribute('stroke'),'#A78BFA');
  await page.getByRole('button',{name:'Slot 1: Office mode, unsaved',exact:true}).click();
  await page.setViewportSize({width:390,height:844});
  const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth);
  assert.equal(overflow,false,'Mobile page must not overflow horizontally');
  await page.screenshot({path:process.env.ONX_UI_SCREENSHOT||'/tmp/onx-config-mobile.png',fullPage:true});
  let nativeDialogs=0;page.on('dialog',dialog=>{nativeDialogs++;dialog.dismiss()});
  // Answer the in-page (shadcn-style) dialog; returns its title.
  const answer=async(value)=>{const box=page.locator('#ask');await box.waitFor({state:'visible'});const title=await page.locator('#ask-title').textContent();if(value!==undefined)await page.locator('#ask-input').fill(value);await page.locator('#ask-ok').click();await box.waitFor({state:'hidden'});return title};
  // Presets live in the Preset card above the slots in Display & actions.
  await page.setViewportSize({width:1100,height:850});
  assert.equal(await page.getByRole('button',{name:'Presets',exact:true}).count(),0,'The Presets tab was merged into Display & actions');
  assert.equal(await page.locator('#preset-toggle').getAttribute('aria-expanded'),'false','Presets start collapsed');
  assert.ok(await page.locator('#preset-body').isHidden());
  assert.equal(await page.locator('#preset-summary').textContent(),'On the knob');
  assert.equal(await page.locator('#preset-badge').textContent(),'Live');
  await page.locator('#preset-toggle').click();
  assert.ok(await page.locator('#preset-body').isVisible());
  assert.match(await page.locator('#preset-mode').textContent(),/on the knob now/);
  assert.equal(await page.locator('#slots-heading').textContent(),'Slots');
  await page.locator('#preset-save-as').click();
  assert.equal(await answer('Evening'),'Save as preset');
  await page.getByRole('status').filter({hasText:'Saved the knob’s controls as “Evening”.'}).waitFor();
  assert.equal(presetOps.at(-1).op,'preset_save');assert.equal(presetOps.at(-1).name,'Evening');
  assert.match(await page.locator('#preset-target option[value="Evening"]').textContent(),/active/);
  await page.locator('#preset-target').selectOption('Evening');
  assert.equal(await answer(),'Discard unsaved changes?');
  await page.getByRole('status').filter({hasText:'Editing preset “Evening”'}).waitFor();
  assert.equal(await page.locator('#preset-badge').textContent(),'Active');
  assert.ok(await page.locator('#preset-save-as').isHidden());
  assert.equal(await page.locator('#preset-apply').getAttribute('aria-label'),'Apply preset to knob');
  await page.locator('#preset-apply').click();
  assert.match(await answer(),/Apply “Evening” to the knob\?/);
  await page.getByRole('status').filter({hasText:'Preset “Evening” applied.'}).waitFor();
  assert.ok(presetOps.some(o=>o.op==='preset_apply'&&o.name==='Evening'));
  assert.equal(await page.locator('#preset-target').inputValue(),'');
  await page.getByRole('button',{name:'Backup & diagnostics',exact:true}).click();
  const [download]=await Promise.all([page.waitForEvent('download'),page.locator('#export').click()]);
  const backup=JSON.parse(fs.readFileSync(await download.path(),'utf8'));
  assert.ok(presetStore.chunks>1,'Presets download in chunks');
  assert.equal(backup.presets.length,1);assert.equal(backup.presets[0].name,'Evening');assert.equal(backup.presets[0].slots[0].entity,'sensor.living_room_temperature');
  backup.presets.push({name:'Weekend',slots:backup.presets[0].slots.map(slot=>({...slot,name:'Estación 🌙 '+'x'.repeat(140)}))});
  await page.locator('#import-file').setInputFiles({name:'backup.json',mimeType:'application/json',buffer:Buffer.from(JSON.stringify(backup))});
  assert.equal(await answer(),'Restore presets from this backup?');
  await page.getByRole('status').filter({hasText:'2 presets restored'}).waitFor();
  assert.ok(presetOps.filter(o=>o.op==='preset_put'&&o.name==='Weekend').length>1,'Large presets upload in chunks');
  assert.ok(presetOps.filter(o=>o.op==='preset_put').every(o=>new URLSearchParams(o).toString().length<=4000),'Chunk requests stay small');
  assert.ok(presetStore.docs.Weekend[0].name.startsWith('Estación 🌙 x'),'Non-ASCII text survives chunking');
  assert.ok(presetStore.presets.some(p=>p.name==='Weekend'));
  presetStore.presets=presetStore.presets.filter(p=>p.name!=='Weekend');
  await page.getByRole('button',{name:'Display & actions',exact:true}).click();
  await page.locator('#preset-target').selectOption('Evening');
  await page.getByRole('status').filter({hasText:'Editing preset “Evening”'}).waitFor();
  await page.locator('#preset-rename').click();
  await page.locator('#ask-input').waitFor({state:'visible'});await page.locator('#ask-input').fill('custom');await page.locator('#ask-ok').click();
  await page.locator('#ask-error').filter({hasText:'reserved'}).waitFor();
  assert.equal(await answer('Night'),'Rename preset');
  await page.getByRole('status').filter({hasText:'Renamed “Evening” to “Night”.'}).waitFor();
  assert.equal(await page.locator('#preset-target').inputValue(),'Night');
  await page.locator('#preset-delete').click();
  assert.match(await answer(),/Delete preset “Night”/);
  await page.getByRole('status').filter({hasText:'Showing the controls on the knob.'}).waitFor();
  assert.equal(presetStore.presets.length,0);assert.equal(await page.locator('#preset-target').inputValue(),'');
  // A new preset is edited and saved without applying it.
  const savesBefore=saved.length;
  await page.getByRole('button',{name:'New preset',exact:true}).click();
  assert.equal(await answer('Guests'),'New preset');
  await page.getByRole('status').filter({hasText:'New preset “Guests”'}).waitFor();
  assert.equal(await page.locator('#preset-badge').textContent(),'New');
  assert.ok(await page.locator('#preset-apply').isDisabled(),'A new preset must be saved before it can be applied');
  assert.match(await page.locator('#slot-title').textContent(),/Slot 1 · Guests/);
  await page.locator('#choose-entity').click();
  await page.locator('#results').getByRole('button',{name:'Office fan',exact:false}).click();
  if(await page.locator('#entity-picker').isVisible())await page.locator('#close-picker').click();
  await page.locator('#save-all').click();
  await page.getByRole('status').filter({hasText:'Preset “Guests” created.'}).waitFor();
  assert.equal(saved.length,savesBefore,'Saving a preset must not save knob slots');
  assert.equal(presetStore.docs.Guests[0].entity,'fan.office');
  assert.ok(!presetOps.some(o=>o.op==='preset_apply'&&o.name==='Guests'),'Saving a preset must not apply it');
  assert.equal(await page.locator('#preset-badge').textContent(),'Saved');
  assert.ok(await page.locator('#preset-apply').isEnabled());
  await page.screenshot({path:path.join(screenshots,'onx-preset-card.png')});
  await page.locator('#preset-target').selectOption('');
  await page.getByRole('status').filter({hasText:'Showing the controls on the knob.'}).waitFor();
  await page.locator('#preset-target').selectOption('Guests');
  await page.getByRole('status').filter({hasText:'Editing preset “Guests”'}).waitFor();
  assert.match(await page.locator('#slots').textContent(),/Office fan|fan\.office/);
  await page.setViewportSize({width:390,height:844});
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth),false,'Preset card must fit on mobile');
  await page.screenshot({path:path.join(screenshots,'onx-preset-card-mobile.png'),fullPage:true});
  assert.equal(nativeDialogs,0,'Use in-page dialogs, not browser ones');
  console.log('Configuration UI passed: search, pairing, save, no-action, failure retry, draft preservation, manual IDs, presets, preset backup, preset editor, in-page dialogs, mobile layout');
 }finally{await browser.close()}
})().catch(error=>{console.error(error);process.exit(1)});
