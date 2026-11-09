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
   if(url.pathname==='/knob/api'){
    const values=new URLSearchParams(request.postData());
    let result;
    if(values.get('op')==='state')result={count:0,slots,weather:'',ha_url:'http://ha.test:8123'};
    else if(values.get('op')==='preview')result={slots:slots.filter(s=>s.entity).map(s=>{
     const media=s.entity.startsWith('media_player.'),climate=s.entity.startsWith('climate.');
     return {entity:s.entity,layout:media?'media':'value',available:true,icon:'mdi:thermometer',icon_x:media?0:-62,icon_y:media?18:32,icon_size:media?80:28,progress:media?470:-1,playing:media,artwork:media?'http://ha.test/artwork.svg':'',lines:[
      {text:media?'':climate?'🔥 Heat':'🌡 Temperature',y:36,size:16,width:156,x:16,color:climate?0xffaa66:0x62b0ff},
      {text:media?'Volume: 50%':climate?'21.5°':'16.63 °C',y:media?166:climate?75:84,size:media?18:32,width:208},
      {text:media?'Silent Night / Noche de Paz':climate?'Now 23°':'',y:media?122:119,size:16,width:196},
      {text:media?'Christina Aguilera':climate?'Press: mode\nHold + turn: setting':'',y:media?144:147,size:media?16:14,width:198},
      {text:s.name||s.entity,y:media?192:185,size:16,width:180}
     ]};
    })};
    else if(values.get('op')==='icon'){iconUpdates.push(Object.fromEntries(values));Object.assign(slots[Number(values.get('slot'))],{icon:values.get('icon'),iconOverride:values.get('icon_override')});result={ok:true}}
    else if(values.get('op')==='settings'){savedSettings.push(Object.fromEntries(values));result={ok:true}}
    else if(values.get('op')==='settings_state')result={brightness:'100',standby:'300',sensitivity:'2',sleep_enabled:'0',sleep_start:'23:00',sleep_end:'07:00',sleep_timeout:'300'};
    else if(values.get('op')==='save'){
     if(failSave)result={error:'Could not save the press action; retry'};
     else {saved.push(Object.fromEntries(values));slots[Number(values.get('slot'))]={entity:values.get('entity'),action:values.get('action')};result={ok:true}}
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
  assert.equal(iconUpdates.length,1);assert.equal(iconUpdates[0].slot,'9');assert.equal(iconUpdates[0].icon,'mdi:ceiling-fan');
  await page.getByRole('button',{name:'Living room temperature'}).waitFor();
  await page.locator('#search').fill('living room');
  await page.getByRole('button',{name:'Living room temperature'}).click();
  await page.locator('#choose-action').click();
  await page.getByRole('button',{name:'Comfort scene'}).click();
  assert.equal(await page.locator('#entity-id').textContent(),'sensor.living_room_temperature');
  assert.equal(await page.locator('#action-name').textContent(),'Comfort scene');
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.equal(saved[0].action,'script.comfort');assert.equal(saved[0].entity,'sensor.living_room_temperature');
  await page.locator('#device-preview text').filter({hasText:'16.63 °C'}).waitFor();
  assert.equal(await page.locator('#device-preview text').filter({hasText:'16.63 °C'}).getAttribute('font-size'),'32');
  await page.getByRole('button',{name:'Slot 2: Empty',exact:true}).click();
  await page.locator('#choose-entity').click();
  await page.getByRole('button',{name:'Front door'}).click();
  await page.locator('#none').click();failSave=true;
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'Could not save the press action'}).waitFor();
  assert.equal(await page.locator('#save').isDisabled(),false);
  failSave=false;await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.equal(saved[1].action,'none');
  await page.getByRole('button',{name:'Slot 1: Living room temperature',exact:true}).click();
  assert.equal(await page.locator('#action-name').textContent(),'Comfort scene');
  await page.locator('#default').click();
  assert.equal(await page.locator('#action-name').textContent(),'Device default');
  await page.locator('#choose-action').click();
  await page.locator('#search').fill('script.manual');
  await page.getByRole('button',{name:'Use entity ID: script.manual'}).waitFor();
  await page.locator('#choose-entity').click();
  await page.locator('#search').fill('Office fan');
  await page.locator('#results').getByRole('button',{name:'Office fan'}).click();
  assert.equal(await page.locator('#choose-action').isDisabled(),true);
  assert.match(await page.locator('#rotate-hint').textContent(),/speed/);
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'1 slot saved.'}).waitFor();
  assert.equal(saved.at(-1).icon,'mdi:ceiling-fan');assert.equal(saved.at(-1).icon_override,'1');
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
  await page.locator('#device-preview text').filter({hasText:'Christina Aguilera'}).waitFor();
  assert.equal(await page.locator('.preview-ring').getAttribute('stroke-dasharray'),'470 530');
  assert.match(await page.locator('#device-preview').textContent(),/Volume: 50%/);
  assert.equal(await page.locator('#device-preview image').last().getAttribute('width'),'100');
  await page.screenshot({path:path.join(screenshots,'onx-media-preview.png'),fullPage:true});
  await page.getByRole('button',{name:'Device settings',exact:true}).click();
  assert.equal(await page.locator('#standby-scope').inputValue(),'main');
  await page.getByRole('button',{name:'Soft violet',exact:true}).click();
  assert.equal(await page.locator('#media-color').inputValue(),'#a78bfa');
  await page.locator('#standby-scope').selectOption('all');
  await page.locator('#save-settings').click();
  await page.getByRole('status').filter({hasText:'Device settings saved.'}).waitFor();
  assert.equal(savedSettings.at(-1).media_color,'#A78BFA');assert.equal(savedSettings.at(-1).standby_scope,'all');
  await page.getByRole('button',{name:'Display & actions',exact:true}).click();
  assert.equal(await page.locator('.preview-ring').getAttribute('stroke'),'#A78BFA');
  await page.getByRole('button',{name:'Slot 1: Office mode, unsaved',exact:true}).click();
  await page.setViewportSize({width:390,height:844});
  const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth);
  assert.equal(overflow,false,'Mobile page must not overflow horizontally');
  await page.screenshot({path:process.env.ONX_UI_SCREENSHOT||'/tmp/onx-config-mobile.png',fullPage:true});
  console.log('Configuration UI passed: search, pairing, save, no-action, failure retry, draft preservation, manual IDs, mobile layout');
 }finally{await browser.close()}
})().catch(error=>{console.error(error);process.exit(1)});
