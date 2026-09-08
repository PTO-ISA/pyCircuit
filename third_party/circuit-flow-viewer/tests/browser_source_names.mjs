// Source-label presentation and internal-identity details on a ROB replay.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [playwright, html, evidence] = process.argv.slice(2);
const {chromium} = await import(pathToFileURL(path.resolve(playwright, 'index.mjs')));
await fs.mkdir(evidence, {recursive:true});
const browser = await chromium.launch({headless:true,args:['--no-sandbox']});
try {
  const page = await browser.newPage({viewport:{width:1800,height:1200}});
  const errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  await page.goto(pathToFileURL(path.resolve(html)).href);
  await page.waitForFunction(()=>window.flowTest);
  const operations = await page.evaluate(()=>flowTest.recording.objects.filter(o=>o.name.startsWith('firing_')));
  assert.equal(operations.length,10);
  for(let instance=0;instance<2;instance++) {
    const parent=`dual_rob_system/rob[${instance}]`;
    assert(await page.locator(`[data-module-path="${parent}"]`).count()>0);
    for(const name of ['recover','acknowledge','complete','handoff','allocate']) {
      const object=operations.find(o=>o.display_path===`${parent}/${name}`);
      assert(object,`${parent}/${name}`);
      assert.equal(object.display_name,name);
      const card=page.locator(`[data-object="${object.id.bits}"]`);
      assert.equal(await card.locator('h2').textContent(),name);
      assert.equal(await card.locator('.path').textContent(),`${parent}/${name}`);
      await card.locator('h2').click();
      const detail=await page.locator('#detail').textContent();
      assert(detail.includes(object.name));
      assert(detail.includes(object.path));
      assert(detail.includes(`stid = ${instance}`));
    }
  }
  await page.evaluate(()=>createCard({id:{bits:'99999',width:64,signed:false},name:'firing_native__effect_9',visual:'operation'}));
  assert.equal(await page.locator('[data-object="99999"] h2').textContent(),'firing_native__effect_9');
  await page.locator('[data-object="99999"]').evaluate(el=>el.remove());
  await page.locator('#operations').scrollIntoViewIfNeeded();
  await page.screenshot({path:path.join(evidence,'source-names.png')});
  assert.deepEqual(errors,[]);
  console.log(JSON.stringify({operations:operations.length,modulePaths:2,fallback:'unchanged',errors}));
} finally {await browser.close();}
