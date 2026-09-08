// Table presentation regression with an independent, small typed-state oracle.
// node browser_table_tree.mjs <playwright> <ROB HTML> <evidence directory>
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [playwright,html,evidence]=process.argv.slice(2);
const {chromium}=await import(pathToFileURL(path.resolve(playwright,'index.mjs')));
await fs.mkdir(evidence,{recursive:true});
const u=(bits,width=16)=>({bits:String(bits),width,signed:false});
const entry=n=>({valid:u(1,1),rob:{flow:{stid:u(n)},slot:u(n,4),gen:u(1)},inst:{slot:u(7,4)},items:[{value:u(9)},{value:u(10)}],result:u('18446744073709551615',64),status:u(2,3)});
const before=[entry(0),entry(1)],after=structuredClone(before);
after[0].rob.flow.stid=u(3);after[0].items[1].value=u(12);after[1].result=u('9007199254740993',64);
const initial={'1':{entries:before},'2':{entries:structuredClone(before)},'3':{entries:[u(0)]}};
const changes={'1':{before:initial['1'],after:{entries:after}},'3':{before:initial['3'],after:{entries:[u(1)]}}};
const fixture={complete:true,objects:[1,2].map(id=>({id:u(id,64),name:'entries'+id,visual:'table',entry:entry(0),flat:false})).concat([{id:u(3,64),name:'counter',visual:'table',entry:u(0)}]),initial,commits:[{time:u(1),delta:u(0),changes}],events:[
  {batch:u(0),object:u(1),owner:u(99),operation:u(1),action:'state_read',index:u(0)},
  ...[0,1].map(i=>({batch:u(0),object:u(1),owner:u(99),operation:u(1),action:'state_write',index:u(i),before:before[i],after:after[i]}))]};
const text=await fs.readFile(html,'utf8');
const fixturePath=path.join(evidence,'table-fixture.html');
await fs.writeFile(fixturePath,text.replace(/const recording = [\s\S]*?;\nconst \$/,`const recording = ${JSON.stringify(fixture)};\nconst $`));
const browser=await chromium.launch({headless:true,args:['--no-sandbox']});
try{
  const page=await browser.newPage({viewport:{width:1800,height:1200}}),errors=[],requests=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('request',r=>requests.push(r.url()));
  await page.goto(pathToFileURL(path.resolve(fixturePath)).href);await page.waitForFunction(()=>window.flowTest);
  const table=page.locator('[data-object="1"]'),other=page.locator('[data-object="2"]');
  const toggle=name=>table.getByRole('button',{name:'Toggle '+name,exact:true});
  const row=i=>table.locator(`tbody tr[data-index="${i}"]`);
  const cell=(i,field)=>row(i).locator(`td[data-field="${field}"]`);
  assert.equal(await table.locator('tbody tr').count(),2);
  assert.equal(await toggle('rob').getAttribute('aria-expanded'),'false');
  assert.equal(await cell(0,'rob').innerText(),'{…}');
  await toggle('rob').focus();await page.keyboard.press('Enter');
  await toggle('rob.flow').click();
  assert.equal(await cell(0,'rob.flow.stid').innerText(),'0');
  assert.equal(await table.locator('thead tr').count(),3);
  assert.equal(await other.getByRole('button',{name:'Toggle rob',exact:true}).getAttribute('aria-expanded'),'false');
  await table.getByRole('button',{name:'Expand all',exact:true}).click();
  assert.equal(await cell(0,'items[1].value').innerText(),'10');
  assert.equal(await table.locator('thead th').filter({hasText:/^slot$/}).count(),2);
  assert(!(await table.locator('thead th').allTextContents()).some(s=>s.includes('.')));
  await cell(0,'result').click();assert.match(await page.locator('#detail').innerText(),/18446744073709551615 \(u64\)/);
  await cell(0,'status').click();assert.match(await page.locator('#detail').innerText(),/2 \(u3\)/);
  await table.locator('.field-picker summary').click();
  const field=name=>table.locator('.field-picker').getByRole('checkbox',{name,exact:true});
  await field('rob.slot').uncheck();
  assert(await field('rob').evaluate(e=>e.indeterminate));
  assert.equal(await cell(0,'rob.slot').count(),0);
  await table.getByRole('button',{name:'Collapse all',exact:true}).click();await toggle('rob').click();
  assert.equal(await cell(0,'rob.slot').count(),0);
  await table.locator('.row-picker summary').click();
  const range=table.getByRole('textbox',{name:'Row range'});
  await range.fill('1');await table.getByRole('button',{name:'Apply range'}).click();
  assert.equal(await row(0).count(),0);assert.equal(await row(1).count(),1);
  for(const invalid of ['0,2','1-0','-1','a','9007199254740999']){
    await range.fill(invalid);await table.getByRole('button',{name:'Apply range'}).click();
    assert.match(await table.locator('[role="alert"]').innerText(),/0 to 1/);assert.equal(await row(0).count(),0);
  }
  assert.equal(await page.locator('.register-value').count(),1);
  await range.fill('0–1');await range.press('Enter');assert.equal(await table.locator('tbody tr').count(),2);
  await table.locator('.row-picker').getByRole('checkbox',{name:'Row 0',exact:true}).uncheck();
  await cell(1,'result').click();
  await page.locator('#next').click();
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),{'1':{entries:after},'2':initial['2'],'3':{entries:[u(1)]}});
  assert.match(await page.locator('#detail').innerText(),/9007199254740993 \(u64\)/);
  assert.equal(await row(0).count(),0);assert.equal(await cell(1,'rob.slot').count(),0);
  assert.match(await table.locator('.table-notice').innerText(),/Hidden row 0/);
  await table.locator('.table-notice button').first().click();assert.equal(await row(0).count(),1);
  // The collapsed flow group reflects its changed descendant.
  assert(await cell(0,'rob.flow').evaluate(e=>e.classList.contains('write-active')));
  await page.locator('#back').click();assert.deepEqual(await page.evaluate(()=>flowTest.getState()),initial);
  await field('rob').check();await field('rob').uncheck();
  await page.locator('#next').click();assert.match(await table.locator('.table-notice').innerText(),/Hidden fields/);
  const position=await page.evaluate(()=>flowTest.getPosition());
  await table.locator('.table-notice button').first().click();
  assert.equal(await page.evaluate(()=>flowTest.getPosition()),position);assert.equal(await cell(0,'rob.flow.stid').innerText(),'3');
  await table.locator('.field-picker').getByRole('button',{name:'Clear',exact:true}).click();
  assert.match(await table.locator('.table-empty').innerText(),/No fields/);
  await table.locator('.field-picker').getByRole('button',{name:'Select all',exact:true}).click();
  await table.locator('.row-picker').getByRole('button',{name:'Clear',exact:true}).click();
  assert.match(await table.locator('.table-empty').innerText(),/No rows/);
  await table.locator('.row-picker').getByRole('button',{name:'Select all',exact:true}).click();
  assert.equal(await other.locator('tbody tr').count(),2);
  // Play visual phases without applying a partial commit; retain the view on pause.
  await page.locator('#back').click();await range.fill('1');await range.press('Enter');
  await page.locator('#speed').selectOption('1600');await page.locator('#play').click();
  await page.waitForFunction(()=>document.querySelector('#phase').textContent.includes('①'));
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),initial);
  assert.match(await table.locator('.table-notice').innerText(),/Hidden row 0.*read/);
  await page.locator('#play').click();await page.waitForTimeout(600);
  assert.equal(await page.evaluate(()=>flowTest.getPosition()),0);assert.equal(await row(0).count(),0);
  await page.locator('#next').click();assert.equal(await row(0).count(),0);
  await table.locator('.row-picker').getByRole('button',{name:'Select all',exact:true}).click();
  await table.locator('.row-picker summary').click();await table.locator('.field-picker summary').click();
  await page.screenshot({path:path.join(evidence,'table-tree.png'),fullPage:true});
  await page.reload();await page.waitForFunction(()=>window.flowTest);
  assert.equal(await toggle('rob').getAttribute('aria-expanded'),'false');assert.equal(await table.locator('tbody tr').count(),2);
  // The same controls must leave every committed projection of a real ROB
  // unchanged, including states of rows and fields excluded from the view.
  await page.goto(pathToFileURL(path.resolve(html)).href);await page.waitForFunction(()=>window.flowTest);
  const rob=page.locator('.table-node:not(.register-node)').first();
  await rob.locator('.field-picker summary').click();
  await rob.locator('.field-picker').getByRole('button',{name:'Clear',exact:true}).click();
  for(const name of ['rob','valid','done','result'])await rob.locator('.field-picker').getByRole('checkbox',{name,exact:true}).check();
  await rob.locator('.field-picker summary').click();
  await rob.locator('.row-picker summary').click();
  await rob.getByRole('textbox',{name:'Row range'}).fill('0,1');
  await rob.getByRole('button',{name:'Apply range'}).click();
  await rob.locator('.row-picker summary').click();
  await rob.getByRole('button',{name:'Toggle rob',exact:true}).click();
  const projection=await page.evaluate(()=>{
    const expected=structuredClone(flowTest.recording.initial);
    for(let i=0;i<flowTest.recording.commits.length;i++){
      for(const[id,c]of Object.entries(flowTest.recording.commits[i].changes))expected[id]=c.after;
      flowTest.goto(i+1);
      if(JSON.stringify(flowTest.getState())!==JSON.stringify(expected))throw Error(`Projection changed at commit ${i+1}`);
    }
    return {count:flowTest.recording.commits.length,final:flowTest.getState()};
  });
  assert.equal(await rob.locator('tbody tr').count(),2);
  assert.equal(await rob.getByRole('button',{name:'Toggle rob',exact:true}).getAttribute('aria-expanded'),'true');
  await page.locator('#back').click();await page.locator('#next').click();
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),projection.final);
  await rob.scrollIntoViewIfNeeded();
  await page.screenshot({path:path.join(evidence,'rob-filtered.png')});
  await page.evaluate(()=>flowTest.goto(0));
  assert.deepEqual(await page.evaluate(()=>flowTest.getState()),await page.evaluate(()=>flowTest.recording.initial));
  assert.deepEqual(errors,[]);assert(requests.every(url=>url.startsWith('file:')));
  console.log(JSON.stringify({tree:true,filters:true,hiddenChanges:true,exactIntegers:true,atomic:true,offline:true,errors}));
}finally{await browser.close();}
