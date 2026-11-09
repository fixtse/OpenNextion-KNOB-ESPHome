const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const {webcrypto}=require('node:crypto');
const header=fs.readFileSync('my_components/onx_config_web/ha_browser.h','utf8');
const js=header.split('R"ONXJS(')[1].split(')ONXJS"')[0];
const states=Array.from({length:12000},(_,i)=>({entity_id:'sensor.test_'+i,attributes:{friendly_name:'Sensor '+i}}));
states.push({entity_id:'climate.calentador',attributes:{friendly_name:'Calentador'}},{entity_id:'media_player.room',attributes:{friendly_name:'Room player'}},{entity_id:'light.rgb',attributes:{supported_color_modes:['rgb']}},{entity_id:'weather.home',attributes:{}},{entity_id:'camera.unsupported',attributes:{}});
const knobDomains=['fan','number','input_number','select','input_select','scene','button','input_button'];
for(const domain of knobDomains)states.push({entity_id:domain+'.test',attributes:{friendly_name:domain+' test'}});
const storage=new Map(),sessionStorage={getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)};
const commands=[],http=[],urls=[];
class Socket {
 constructor(url){urls.push(String(url));queueMicrotask(()=>this.onmessage({data:JSON.stringify({type:'auth_required'})}))}
 send(text){const request=JSON.parse(text);commands.push(request.type);
  if(request.type==='auth'){assert.equal(request.access_token,'browser-secret');queueMicrotask(()=>this.onmessage({data:JSON.stringify({type:'auth_ok'})}));return}
  const result=request.type==='get_states'?states:request.type==='config/entity_registry/list_for_display'?{entities:[{ei:'climate.calentador',di:'heater'}]}:request.type==='config/device_registry/list'?[{id:'heater',area_id:'kitchen'}]:[{area_id:'kitchen',name:'Kitchen'}];
  queueMicrotask(()=>this.onmessage({data:JSON.stringify({id:request.id,type:'result',success:true,result})}));
 }
 close(){this.onclose?.()}
}
const context={URL,URLSearchParams,crypto:webcrypto,TextEncoder,btoa:s=>Buffer.from(s,'binary').toString('base64'),setTimeout,clearTimeout,WebSocket:Socket,window:{sessionStorage},location:{origin:'http://192.168.10.52',search:'',assign:url=>urls.push(url)},history:{replaceState(){}},fetch:async(url,options)=>{http.push({url,options});return {ok:true,json:async()=>({access_token:'browser-secret',refresh_token:'refresh-secret',expires_in:1800})}}};
vm.runInNewContext(js,context);const {catalogue,search,Client,baseUrl}=context.OnxHA;
(async()=>{
 const client=new Client(sessionStorage);sessionStorage.setItem('onx.ha.auth',JSON.stringify({base:'http://ha.local:8123',access:'browser-secret',refresh:'refresh-secret',expires:Date.now()+60000}));
 const items=await client.load();assert.equal(items.length,12016);assert(client.ready);assert.equal(http.length,0);assert(urls.every(u=>u.startsWith('ws://ha.local:8123/')));
 for(const domain of knobDomains)assert.equal(search(items,'',domain).total,1);
 const match=search(items,'calentador','climate');assert.equal(match.total,1);assert.equal(match.items[0].area,'Kitchen');
 assert.equal(search(items,'','sensor').total,12000);assert.equal(search(items,'','sensor',30).items.length,30);
 assert.equal(search(items,'','light').items[0].type,'RGB Light');
 assert.equal(search(items,'','switch').total,4);assert.equal(search(items,'','actions').total,0);
 sessionStorage.setItem('onx.ha.auth',JSON.stringify({base:'http://ha.local:8123',access:'expired',refresh:'refresh-secret',expires:0}));
 await client.load();assert.equal(http.length,1);assert.equal(http[0].url,'http://ha.local:8123/auth/token');assert(!http.some(r=>r.url.includes('192.168.10.52')));
 client.disconnect();assert.equal(sessionStorage.getItem('onx.ha.auth'),null);assert.equal(client.items.length,4);await assert.rejects(()=>client.token(),/Connect/);
 context.location.search='?code=bogus&state=invalid';await assert.rejects(()=>client.finishLogin(),/expired/);assert.equal(http.length,1);
 context.location.search='';await client.login('http://ha.local:8123');const flow=JSON.parse(sessionStorage.getItem('onx.ha.flow'));assert(flow.state.length>=32);assert(flow.verifier);assert(urls.at(-1).includes('code_challenge='));
 context.location.search='?code=valid&state='+flow.state;await client.finishLogin();assert.equal(http.length,2);assert(http[1].options.body.has('code_verifier'));assert.equal(sessionStorage.getItem('onx.ha.flow'),null);
 assert.throws(()=>baseUrl('javascript:alert(1)'));assert.throws(()=>baseUrl('https://user:pass@ha.local'));
 const page=fs.readFileSync('my_components/onx_config_web/config_page.h','utf8');for(const script of page.matchAll(/<script>([\s\S]*?)<\/script>/g))new vm.Script(script[1]);
 assert(!page.includes("op:'search'"));assert(!page.includes("op:'refresh'"));
 const interval=fs.readFileSync('packages/features/normal_mode/normal_mode_interval.yaml','utf8');assert(!interval.includes('maintain_ha_websocket('));
 const source=fs.readFileSync('custom_logic.h','utf8');assert(source.includes('MAX_HA_ENTITY_COUNT = 21;'));assert(source.includes('ha-selected-entities-v5-chunk'));
 const selects=fs.readFileSync('packages/features/normal_mode/normal_mode_entities.yaml','utf8').split('\nselect:\n')[1].split('\ntext_sensor:')[0];assert.equal((selects.match(/internal: true/g)||[]).length,21);
 console.log('Browser catalogue: 12,000 entities, metadata, paging, OAuth/PKCE, token refresh, device isolation passed');
})().catch(error=>{console.error(error);process.exitCode=1});
