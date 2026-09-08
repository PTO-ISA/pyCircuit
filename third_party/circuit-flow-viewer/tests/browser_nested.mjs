// Generic nested-record browser regression; accepts a producer-generated trace page.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [playwright, html, evidence] = process.argv.slice(2);
const {chromium} = await import(pathToFileURL(path.resolve(playwright, 'index.mjs')));
await fs.mkdir(evidence, {recursive:true});
const browser = await chromium.launch({headless:true,args:['--no-sandbox']});
try {
  const page = await browser.newPage({viewport:{width:1600,height:1100}});
  const errors=[], requests=[];
  page.on('pageerror',e=>errors.push(e.message));
  page.on('request',r=>requests.push(r.url()));
  await page.goto(pathToFileURL(path.resolve(html)).href);
  await page.waitForFunction(()=>window.flowTest);
  assert.equal(await page.locator('.unsupported').count(),0);
  assert(await page.locator('.queue-node .slot').count()>0);
  const nested=await page.evaluate(()=>flowTest.recording.objects.find(o=>o.visual==='table' && o.flat===false).id.bits);
  const table=page.locator(`[data-object="${nested}"]`);
  assert(await table.getByRole('button',{name:'Toggle rob',exact:true}).count()>0);
  await table.getByRole('button',{name:'Expand all',exact:true}).click();
  assert((await table.locator('thead tr').count())>1);
  assert(!(await table.locator('thead th').allTextContents()).some(s=>s.includes('.')));
  const initial=await page.evaluate(()=>flowTest.getState());
  await page.locator('#next').click();assert.equal(await page.evaluate(()=>flowTest.getPosition()),1);
  await page.locator('#back').click();assert.deepEqual(await page.evaluate(()=>flowTest.getState()),initial);
  const target=await page.evaluate(()=>{
    const index=flowTest.recording.commits.findIndex(c=>Object.keys(c.changes).length>2);
    if(index<0)throw Error('Need an atomic multi-owner commit');
    flowTest.goto(index);return index;
  });
  const before=await page.evaluate(()=>flowTest.getState());
  await page.locator('#speed').selectOption('1600');
  await page.locator('#play').click();
  await page.waitForFunction(()=>document.querySelector('#phase').textContent.includes('①'));
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),before);
  await page.locator('#play').click();
  const paused=await page.evaluate(()=>flowTest.getPosition());
  await page.waitForTimeout(600);
  assert.equal(await page.evaluate(()=>flowTest.getPosition()),paused);
  await page.evaluate(n=>flowTest.goto(n+1),target);
  const expected=await page.evaluate(n=>{
    const s=structuredClone(flowTest.recording.initial);
    for(const c of flowTest.recording.commits.slice(0,n+1))for(const[id,v]of Object.entries(c.changes))s[id]=v.after;
    return s;
  },target);
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),expected);
  // Find a committed nested row holding a wide exact integer and compare its
  // displayed cell and detail against the original typed value, without Number.
  const selection=await page.evaluate(id=>{
    const typed=v=>v&&typeof v==='object'&&'bits'in v&&'width'in v;
    function find(v,p='') {
      if(typed(v))return v.width===64&&BigInt(v.bits)>9007199254740991n?{field:p,value:v}:null;
      if(!v||typeof v!=='object')return null;
      for(const[k,c]of Object.entries(v)){const r=find(c,p?`${p}.${k}`:k);if(r)return r;}
      return null;
    }
    for(let n=0;n<flowTest.recording.commits.length;n++){
      flowTest.goto(n+1);const entries=flowTest.getState()[id].entries;
      for(let i=0;i<entries.length;i++){const r=find(entries[i]);if(r)return {...r,row:i};}
    }
    throw Error('Need a nested u64 value above 2^53');
  },nested);
  const row=table.locator(`tr[data-index="${selection.row}"]`);
  const cell=row.locator(`td[data-field="${selection.field}"]`);
  assert.equal(await cell.innerText(),selection.value.bits);
  await row.locator('th').click();
  assert((await page.locator('#detail').innerText()).includes(selection.value.bits+' (u64)'));
  assert.match(await page.locator('#detail').innerText(),/\(u(?:3|32)\)/);
  await page.screenshot({path:path.join(evidence,'nested-fields.png')});
  await page.evaluate(()=>flowTest.goto(flowTest.recording.commits.length));
  const final=await page.evaluate(()=>flowTest.getState());
  await page.locator('#back').click();await page.locator('#next').click();
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),final);
  await page.evaluate(()=>flowTest.goto(0));
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),initial);
  assert.deepEqual(errors,[]);assert(requests.every(url=>url.startsWith('file:')));
  console.log(JSON.stringify({nested,field:selection.field,exact:selection.value,offline:true,errors}));
} finally {await browser.close();}
