const assert = require('node:assert/strict');
const fs = require('node:fs');
const { chromium } = require('playwright');

(async () => {
 const browser = await chromium.launch({headless:true, args:['--no-sandbox']});
 try {
  const page = await browser.newPage({viewport:{width:1100,height:850}});
  const html = fs.readFileSync('my_components/onx_config_web/config_page.h','utf8').split('R"ONXHTML(')[1].split(')ONXHTML";')[0];
  const entities = [
   {entity:'sensor.living_room_temperature',name:'Living room temperature',area:'Living room'},
   {entity:'binary_sensor.front_door',name:'Front door',area:'Hallway'},
   {entity:'script.comfort',name:'Comfort scene',area:'Living room'},
   {entity:'automation.welcome',name:'Welcome home',area:'Hallway'}
  ];
  const slots = Array.from({length:10},()=>({entity:'',name:'',action:''}));
  const results = new Map(); let ticket=0, failSave=false, saved=[];
  await page.route('http://knob.test/**',async route=>{
   const request=route.request(),url=new URL(request.url());
   if(url.pathname==='/'||url.pathname==='/knob'){await route.fulfill({contentType:'text/html',body:html});return}
   if(url.pathname==='/knob/api'){
    const values=new URLSearchParams(request.postData());
    let result;
    if(values.get('op')==='state')result={count:entities.length,slots};
    else if(values.get('op')==='search'){
     const domain=values.get('domain'),q=values.get('q').toLowerCase();
     const items=entities.filter(e=>(!domain||(domain==='actions'?/^(script|automation)\./.test(e.entity):e.entity.startsWith(domain+'.')))&&JSON.stringify(e).toLowerCase().includes(q));
     result={items,total:items.length,next:-1};
    }else if(values.get('op')==='save'){
     if(failSave)result={error:'Could not save the press action; retry'};
     else {saved.push(Object.fromEntries(values));slots[Number(values.get('slot'))]={entity:values.get('entity'),action:values.get('action')};result={ok:true}}
    }else result={ok:true};
    results.set(++ticket,result);await route.fulfill({status:202,json:{ticket}});return;
   }
   if(url.pathname==='/knob/result'){await route.fulfill({json:results.get(Number(url.searchParams.get('ticket')))});return}
   throw Error('Unexpected request: '+request.url());
  });
  await page.goto('http://knob.test/');
  await page.getByRole('button',{name:'Living room temperature'}).waitFor();
  await page.locator('#search').fill('living room');
  await page.getByRole('button',{name:'Living room temperature'}).click();
  await page.locator('#choose-action').click();
  await page.getByRole('button',{name:'Comfort scene'}).click();
  assert.equal(await page.locator('#entity-id').textContent(),'sensor.living_room_temperature');
  assert.equal(await page.locator('#action-name').textContent(),'Comfort scene');
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'Slot 1 saved'}).waitFor();
  assert.equal(saved[0].action,'script.comfort');assert.equal(saved[0].entity,'sensor.living_room_temperature');
  await page.getByRole('button',{name:'Slot 2: Empty',exact:true}).click();
  await page.locator('#choose-entity').click();
  await page.getByRole('button',{name:'Front door'}).click();
  await page.locator('#none').click();failSave=true;
  await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'Could not save the press action'}).waitFor();
  assert.equal(await page.locator('#save').isDisabled(),false);
  failSave=false;await page.locator('#save').click();
  await page.getByRole('status').filter({hasText:'Slot 2 saved'}).waitFor();
  assert.equal(saved[1].action,'none');
  await page.getByRole('button',{name:'Slot 1: Living room temperature',exact:true}).click();
  assert.equal(await page.locator('#action-name').textContent(),'Comfort scene');
  await page.locator('#default').click();
  assert.equal(await page.locator('#action-name').textContent(),'Device default');
  await page.locator('#choose-action').click();
  await page.locator('#search').fill('script.manual');
  await page.getByRole('button',{name:'Use entity ID: script.manual'}).waitFor();
  await page.setViewportSize({width:390,height:844});
  const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth);
  assert.equal(overflow,false,'Mobile page must not overflow horizontally');
  await page.screenshot({path:process.env.ONX_UI_SCREENSHOT||'/tmp/onx-config-mobile.png',fullPage:true});
  console.log('Configuration UI passed: search, pairing, save, no-action, failure retry, draft preservation, manual IDs, mobile layout');
 }finally{await browser.close()}
})().catch(error=>{console.error(error);process.exit(1)});
