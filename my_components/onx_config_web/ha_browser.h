#pragma once
namespace esphome::onx_config_web {
static const char HA_BROWSER_JS[] = R"ONXJS(
(()=>{
 const supported=/^(sensor|binary_sensor|light|switch|climate|media_player|timer|weather|cover|automation|script|fan|number|input_number|select|input_select|scene|button|input_button)\./;
 const local=()=>[10,11,17,18].map(pin=>({entity:'[Local] HMI IO'+pin,name:'HMI IO'+pin,area:'On this knob'}));
 const validIcon=value=>typeof value==='string'&&/^mdi:[a-z0-9-]{1,76}$/.test(value);
 function translatedIcon(definition,state){
  const exact=definition?.state?.[state];
  return validIcon(exact)?exact:validIcon(definition?.default)?definition.default:'';
 }
 function catalogue(states,registry=[],areas=[],devices=[],icons={}){
  const records=new Map((Array.isArray(registry)?registry:registry.entities||[]).map(e=>[e.entity_id||e.ei,e]));
  const rooms=new Map(areas.map(a=>[a.area_id,a.name]));const deviceRooms=new Map(devices.map(d=>[d.id,d.area_id]));
  return states.filter(s=>supported.test(s.entity_id)).map(s=>{
   const r=records.get(s.entity_id)||{},a=s.attributes||{},m=a.supported_color_modes||[];
   const types=[];
   if(s.entity_id.startsWith('light.')){
    types.push('Light (On/Off)');
    if(m.some(v=>['brightness','color_temp','white','rgb','rgbw','rgbww','xy','hs'].includes(v)))types.push('Dimmable Light');
    if(m.includes('color_temp'))types.push('Color Temperature Light');
    if(m.some(v=>['rgb','rgbw','rgbww','xy','hs'].includes(v)))types.push('RGB Light');
   }else if(s.entity_id.startsWith('cover.')){types.push('Cover (Preset Off)');if((Number(a.supported_features)||0)&4)types.push('Cover (Preset On)');}
   const type=types.at(-1)||'';
   const registryIcon=r.ic||r.icon||'';
   const domain=s.entity_id.split('.')[0],platform=r.pl||r.platform,key=r.tk||r.translation_key;
   const component=icons.entity_component?.[domain];
   const inferred=translatedIcon(icons.entity?.[platform]?.[domain]?.[key],s.state)||
    translatedIcon(component?.[a.device_class]||component?._,s.state);
   const icon=validIcon(registryIcon)?registryIcon:validIcon(a.icon)?a.icon:inferred;
   return {entity:s.entity_id,icon,iconOverride:icon&&icon===registryIcon?'1':'0',name:a.friendly_name||r.en||r.name||s.entity_id,area:rooms.get(r.ai||r.area_id||deviceRooms.get(r.di||r.device_id))||'',type,types:types.length?types:undefined};
  }).concat(local()).sort((a,b)=>(a.area+' '+a.name+' '+a.entity).localeCompare(b.area+' '+b.name+' '+b.entity));
 }
 function search(items,q,domain,offset=0){
  const words=q.toLocaleLowerCase(); const matches=items.filter(i=>{
   const d=i.entity.startsWith('[Local] ')?'switch':i.entity.split('.')[0];return (!domain||domain===d||(domain==='actions'&&['script','automation'].includes(d)))&&(!words||(i.entity+' '+i.name+' '+i.area).toLocaleLowerCase().includes(words));
  }); return {items:matches.slice(offset,offset+30),total:matches.length,next:offset+30<matches.length?offset+30:-1};
 }
 function baseUrl(value){const u=new URL(value.includes('://')?value:'http://'+value);if(!['http:','https:'].includes(u.protocol)||u.username||u.password)throw Error('Enter a Home Assistant HTTP or HTTPS address');return u.origin;}
 class Client{
  constructor(storage=window.sessionStorage){this.storage=storage;this.items=local();this.ready=false;}
  read(key){try{return JSON.parse(this.storage.getItem(key)||'null')}catch{return null}}
  async login(value){
   const base=baseUrl(value);const state=Array.from(crypto.getRandomValues(new Uint8Array(24)),x=>x.toString(16).padStart(2,'0')).join('');
   const flow={base,state,time:Date.now()};const params=new URLSearchParams({client_id:location.origin+'/',redirect_uri:location.origin+'/knob',state});
   if(crypto.subtle){flow.verifier=Array.from(crypto.getRandomValues(new Uint8Array(32)),x=>x.toString(16).padStart(2,'0')).join('');const hash=new Uint8Array(await crypto.subtle.digest('SHA-256',new TextEncoder().encode(flow.verifier)));params.set('code_challenge',btoa(String.fromCharCode(...hash)).replace(/\+/g,'-').replace(/\//g,'_').replace(/=+$/,''));params.set('code_challenge_method','S256');}
   this.storage.setItem('onx.ha.flow',JSON.stringify(flow));location.assign(base+'/auth/authorize?'+params);
  }
  async finishLogin(){
   const params=new URLSearchParams(location.search);if(!params.has('code'))return;
   const flow=this.read('onx.ha.flow');history.replaceState(null,'','/knob');this.storage.removeItem('onx.ha.flow');
   if(!flow||params.get('state')!==flow.state||Date.now()-flow.time>900000)throw Error('Home Assistant sign-in expired. Connect again.');
   const body={grant_type:'authorization_code',code:params.get('code'),client_id:location.origin+'/'};if(flow.verifier)body.code_verifier=flow.verifier;
   await this.exchange(flow.base,body);
  }
  async exchange(base,body){
   const response=await fetch(base+'/auth/token',{method:'POST',credentials:'omit',cache:'no-store',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(body)});
   if(!response.ok)throw Error('Home Assistant sign-in failed. Connect again.');const token=await response.json();
   const old=this.read('onx.ha.auth');this.auth={base,access:token.access_token,refresh:token.refresh_token||old?.refresh,expires:Date.now()+token.expires_in*1000};
   this.storage.setItem('onx.ha.auth',JSON.stringify(this.auth));return this.auth;
  }
  async token(){const auth=this.read('onx.ha.auth');if(!auth)throw Error('Connect to Home Assistant to load entities in this browser.');return auth.expires>Date.now()+30000?auth:await this.exchange(auth.base,{grant_type:'refresh_token',refresh_token:auth.refresh,client_id:location.origin+'/'});}
  async load(){
   const auth=await this.token();const url=new URL(auth.base+'/api/websocket');url.protocol=url.protocol==='https:'?'wss:':'ws:';
   this.ready=false;const socket=new WebSocket(url);let id=0;const requests=new Map();
   const call=(type,params={})=>new Promise((resolve,reject)=>{const key=++id;requests.set(key,{resolve,reject});socket.send(JSON.stringify({id:key,type,...params}));});
   try{
    await new Promise((resolve,reject)=>{
     const timer=setTimeout(()=>reject(Error('Home Assistant connection timed out')),15000);
     socket.onerror=()=>{clearTimeout(timer);reject(Error('Cannot connect to Home Assistant from this browser'));};
     socket.onclose=()=>{clearTimeout(timer);reject(Error('Home Assistant disconnected during sign-in'));};
     socket.onmessage=event=>{let data;try{data=JSON.parse(event.data)}catch{clearTimeout(timer);reject(Error('Invalid Home Assistant response'));return}
      if(data.type==='auth_required')socket.send(JSON.stringify({type:'auth',access_token:auth.access}));
      else if(data.type==='auth_ok'){clearTimeout(timer);resolve()}
      else if(data.type==='auth_invalid'){clearTimeout(timer);this.storage.removeItem('onx.ha.auth');reject(Error('Home Assistant sign-in expired. Connect again.'))}
     };
    });
    socket.onmessage=event=>{let data;try{data=JSON.parse(event.data)}catch{for(const p of requests.values())p.reject(Error('Invalid Home Assistant response'));requests.clear();socket.close();return}const pending=requests.get(data.id);if(!pending)return;requests.delete(data.id);data.success?pending.resolve(data.result):pending.reject(Error(data.error?.message||'Home Assistant request failed'));};
    socket.onclose=()=>{for(const p of requests.values())p.reject(Error('Home Assistant disconnected'));requests.clear();};
    const timeout=setTimeout(()=>socket.close(),30000);
    try{const [states,registry,areas,devices]=await Promise.all([call('get_states'),call('config/entity_registry/list_for_display').catch(()=>[]),call('config/area_registry/list').catch(()=>[]),call('config/device_registry/list').catch(()=>[])]);const records=Array.isArray(registry)?registry:registry.entities||[];
     const platforms=[...new Set(records.filter(r=>r.tk||r.translation_key).map(r=>r.pl||r.platform).filter(Boolean))];
     const [entity,component]=await Promise.all([
      platforms.length?call('frontend/get_icons',{category:'entity',integration:platforms}).catch(()=>({})):Promise.resolve({}),
      call('frontend/get_icons',{category:'entity_component'}).catch(()=>({}))]);
     this.items=catalogue(states,registry,areas,devices,{entity:entity.resources,entity_component:component.resources});this.ready=true;return this.items;}finally{clearTimeout(timeout)}
   }finally{socket.close();}
  }
  disconnect(){this.storage.removeItem('onx.ha.auth');this.items=local();this.ready=false;}
 }
 globalThis.OnxHA={catalogue,search,baseUrl,Client};
})();
)ONXJS";
}
