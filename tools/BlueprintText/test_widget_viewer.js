// DOM contract smoke test for the generic viewer; no browser or game data.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const texture="Texture2D'/Game/Test/Icon.Icon'";
const data={asset:'/Game/Test/HUD',root:'root',warnings:[],textures:{[texture]:{
 src:'data:image/png;base64,synthetic',width:100,height:100}},widgets:[
 {id:'root',name:'Root',kind:'CanvasPanel',props:{},raw:{},slot:{},parent:null,children:['panel']},
 {id:'panel',name:'Panel',kind:'HorizontalBox',props:{Visibility:'Collapsed'},raw:{},
  slot:{LayoutData:{Offsets:{Left:10,Top:20,Right:100,Bottom:30}}},parent:'root',children:['icon']},
 {id:'icon',name:'Icon',kind:'Image',props:{Brush:{ResourceObject:texture,ImageSize:{X:30,Y:30},
  UVRegion:{bIsValid:true,Min:{X:.2,Y:.3},Max:{X:.5,Y:.7}}}},raw:{},slot:{},parent:'panel',children:[]}
]};
const html=fs.readFileSync(__dirname+'/widget_viewer.html','utf8');
function element(){const classes=new Set();return {dataset:{},style:{},children:[],attributes:{},classList:{
 toggle(name,on){if(on)classes.add(name);else classes.delete(name)},contains(name){return classes.has(name)}},
 setAttribute(k,v){this.attributes[k]=v},append(...v){this.children.push(...v)},replaceChildren(){this.children=[]},addEventListener(){}}}
function createViewer(data){
const source=html.replace('__DATA__',JSON.stringify(data)).match(/<script>([\s\S]*?)<\/script>/)[1];
const ids=Object.fromEntries(['reset-visibility','stage','asset','viewport','guides','textures','tree','selection','summary','details','metrics','clear','warnings',
 'review-editor','edit-text','edit-color','edit-opacity','opacity-value','move-info','reset-widget','review-output','review-count','copy-review','reset-review','review-status'].map(k=>[k,element()]));
ids.viewport.value='1920,1080';ids.guides.checked=true;ids.textures.checked=true;
const document={getElementById:k=>ids[k],createElement:element,createElementNS:(ns,tag)=>Object.assign(element(),{tag}),addEventListener(){}};
// SVG displayed at half size, with a viewport offset. Pointer movement must use inverse CTM.
ids.stage.getScreenCTM=()=>({inverse:()=>({scale:2,left:50,top:25})});
ids.stage.createSVGPoint=()=>({x:0,y:0,matrixTransform(m){return {x:(this.x-m.left)*m.scale,y:(this.y-m.top)*m.scale}}});
let captured=null;
ids.stage.setPointerCapture=id=>{captured=id};ids.stage.hasPointerCapture=id=>captured===id;ids.stage.releasePointerCapture=()=>{captured=null};
const ctx=vm.createContext({document});vm.runInContext(source,ctx);
const evaluate=expr=>vm.runInContext(expr,ctx);
return {ctx,ids,evaluate,get captured(){return captured}};
}
const viewer=createViewer(data),{ctx,ids,evaluate}=viewer;
assert.equal(evaluate('boxes.size'),1);
assert.equal(ids.tree.children.length,3,'All widgets can be controlled, including hidden descendants');
ctx.select('icon');assert.equal(evaluate('boxes.size'),1,'Selection does not reveal hidden widgets');
ctx.setVisible('panel',true);assert.equal(evaluate('boxes.size'),3);
assert.equal(evaluate("treeRows.get('icon').check.checked"),true);
function all(n){return [n,...n.children.flatMap(all)]}
assert.equal(all(ids.stage).filter(n=>n.tag==='image').length,1);
const crop=all(ids.stage).find(n=>n.tag==='svg');
const bounds=crop.attributes.viewBox.split(' ').map(Number);
for(const [index,value] of [20,30,30,40].entries())assert.ok(Math.abs(bounds[index]-value)<1e-9,'Brush UV crop');
ctx.setVisible('panel',false);assert.equal(evaluate('boxes.size'),1);
assert.equal(evaluate("treeRows.get('icon').check.checked"),true,'Parent preserves child state');
ctx.setVisible('icon',false);ctx.setVisible('panel',true);
assert.equal(evaluate('boxes.size'),2,'Child override survives parent toggle');
evaluate("treeRows.get('icon').check.checked=true;treeRows.get('icon').check.onchange()");
assert.equal(evaluate('boxes.size'),3);
ids.textures.checked=false;ids.textures.onchange();assert.equal(all(ids.stage).filter(n=>n.tag==='image').length,0);
ctx.setVisible('root',false);assert.equal(evaluate('boxes.size'),0);
ids['reset-visibility'].onclick();assert.equal(evaluate('boxes.size'),1);
assert.equal(evaluate('visibilityOverrides.size'),0);
assert.equal(evaluate('selected'),'icon');
ids.clear.onclick();assert.equal(evaluate('selected'),null);
assert.equal(evaluate("widgets.get('panel').props.Visibility"),'Collapsed','Source properties are unchanged');
const original=evaluate('JSON.stringify(data)');
ctx.setVisible('panel',true);ctx.select('icon');
ids['edit-color'].oninput({target:{value:'#8080ff'}});ids['edit-opacity'].oninput({target:{value:'.35'}});
assert.equal(evaluate("adjustments.get('icon').opacity"),.35);
assert.ok(Math.abs(evaluate("adjustments.get('icon').rgb.R")-.21586)<.00001,'sRGB picker converts to Unreal linear RGB');
assert.equal(ids['edit-color'].value,'#8080ff');
let notes=ctx.reviewNotes();assert.equal(notes.changes.length,1);
assert.equal(notes.changes[0].properties[0].after.B,1);
assert.equal(notes.changes[0].properties[1].path,'RenderOpacity');
assert.equal(notes.changes[0].properties[1].before,1);
assert.equal(evaluate('JSON.stringify(data)'),original,'Review edits never mutate exported source');
const pointer=(x,y)=>({button:0,pointerId:7,clientX:x,clientY:y,preventDefault(){},stopPropagation(){}});
ctx.startDrag(pointer(100,100),'panel');ctx.moveDrag(pointer(110,105));ctx.endDrag(pointer(110,105));
assert.equal(viewer.captured,null);
assert.equal(evaluate("adjustments.get('panel').move.dx"),20,'Drag uses layout units, not screen pixels');
assert.equal(evaluate("adjustments.get('panel').move.dy"),10);
assert.equal(evaluate("boxes.get('panel').x"),30);
assert.equal(evaluate("boxes.get('icon').x"),30,'Container moves its child once');
ctx.startDrag(pointer(110,105),'panel');ctx.moveDrag(pointer(120,115));ctx.endDrag(null,true);
assert.equal(evaluate("adjustments.get('panel').move.dx"),20,'Cancel restores previous adjustment');
ctx.startDrag(pointer(100,100),'icon');ctx.moveDrag(pointer(101,100));ctx.endDrag(pointer(101,100));
assert.equal(evaluate("adjustments.get('icon').move"),undefined,'Click jitter is not a move');
ctx.setMove('icon',4,5);assert.equal(evaluate("boxes.get('icon').x"),34);
assert.equal(ctx.reviewNotes().changes.length,2,'Inherited child motion is not an extra change');
ctx.setOpacity('panel',.5);assert.equal(evaluate("opacityOf(widgets.get('icon'))"),.175);
ctx.select('icon');ids['reset-widget'].onclick();assert.equal(evaluate("adjustments.has('icon')"),false);
ids['reset-review'].onclick();assert.equal(ctx.reviewNotes().changes.length,0);
assert.equal(evaluate('JSON.stringify(data)'),original);
ctx.setMove('icon',1,0);ctx.setMove('icon',0,0);assert.equal(evaluate('adjustments.size'),0);
ctx.setTint('icon','#000000');ctx.setTint('icon','#ffffff');assert.equal(evaluate('adjustments.size'),0,'Restoring base tint removes the request');
// Text uses the same controls but its native property is a SlateColor.
evaluate("data.widgets.push({id:'text',name:'Text',kind:'TextBlock',parent:'root',children:[],slot:{},raw:{},props:{Text:'Hello',ColorAndOpacity:{SpecifiedColor:{R:1,G:1,B:1,A:.8},ColorUseRule:'UseColor_Foreground'}}});widgets.set('text',data.widgets.at(-1));widgets.get('root').children.push('text')");
ctx.setTint('text','#ff0000');ctx.setOpacity('text',.5);
const textChange=ctx.reviewNotes().changes.find(c=>c.widgetId==='text');
assert.equal(textChange.properties[0].path,'ColorAndOpacity.SpecifiedColor');
assert.equal(textChange.properties[0].after.A,.8,'Preserve source color alpha');
assert.equal(textChange.properties[1].after,'UseColor_Specified');
assert.ok(all(ids.stage).some(n=>n.tag==='text'&&n.attributes.fill==='rgba(255,0,0,0.8)'));
ctx.select('icon');assert.equal(ids['edit-text'].disabled,true);
ctx.select('text');assert.equal(ids['edit-text'].disabled,false);assert.equal(ids['edit-text'].value,'Hello');
const sourceBeforeText=evaluate('JSON.stringify(data)');
ids['edit-text'].oninput({target:{value:'Target <unknown>\n125 m'}});
assert.equal(ids['edit-text'].value,'Target <unknown>\n125 m');
assert.ok(all(ids.stage).some(n=>n.tag==='tspan'&&n.textContent==='Target <unknown>'),'Text is literal, never HTML');
assert.equal(evaluate("desired(widgets.get('text')).h"),60,'Multiline sample updates automatic height');
let sample=ctx.reviewNotes().changes.find(c=>c.widgetId==='text');
assert.equal(sample.previewText.example,'Target <unknown>\n125 m');
assert.equal(sample.previewText.before,'Hello');
assert.ok(sample.previewText.intent.includes('Preserve dynamic text'));
assert.ok(!sample.properties.some(p=>p.path==='Text'),'Preview sample is not a runtime replacement');
ctx.setText('text','');assert.equal(evaluate("textOf(widgets.get('text'))"),'','An empty override is valid');
ctx.setText('text','Hello');assert.equal(evaluate("adjustments.get('text').text"),undefined);
assert.equal(evaluate('JSON.stringify(data)'),sourceBeforeText,'Text samples never change source');
// Dynamic fields commonly have no exported Text at all.
evaluate("delete widgets.get('text').props.Text");ctx.select('text');assert.equal(ids['edit-text'].value,'');
ctx.setText('text','Dynamic example');assert.equal(evaluate("textOf(widgets.get('text'))"),'Dynamic example');
ids['reset-widget'].onclick();assert.equal(ids['edit-text'].value,'');
assert.equal(evaluate("widgets.get('text').props.Text"),undefined);
ctx.setText('text','Another example');
ids['review-output'].focus=()=>{};let fallbackSelected=false;ids['review-output'].select=()=>{fallbackSelected=true};
// Hidden consumes layout space, including nested children and padding; Collapsed does not.
for(const kind of ['HorizontalBox','VerticalBox']){
 const vertical=kind==='VerticalBox';
 const widget=(id,kind,parent,children,props={},slot={})=>({id,name:id,kind,parent,children,props,slot,raw:{}});
 const icon=(id,parent,width,height,Visibility='Visible')=>widget(id,'Image',parent,[],{Visibility,Brush:{ImageSize:{X:width,Y:height}}});
 const fixture={asset:'/Game/Test/Visibility',root:'root',textures:{},warnings:[],widgets:[
  widget('root',kind,null,['before','panel','after']),icon('before','root',10,8),
  widget('panel',vertical?'HorizontalBox':'VerticalBox','root',['child','hiddenChild','collapsedChild'],{Visibility:'Hidden'},
   {Padding:{Left:2,Right:3,Top:4,Bottom:5}}),
  icon('child','panel',20,12),icon('hiddenChild','panel',30,18,'Hidden'),icon('collapsedChild','panel',99,99,'Collapsed'),
  icon('after','root',40,10)
 ]};
 const v=createViewer(fixture),e=v.evaluate;
 const source=e('JSON.stringify(data)'),axis=vertical?'y':'x',after=()=>e(`boxes.get('after').${axis}`);
 const painted=()=>all(v.ids.stage).filter(n=>n.attributes['data-widget']).map(n=>n.attributes['data-widget']);
 const selectionMarks=()=>all(v.ids.stage).filter(n=>['chosen','move-handle'].includes(n.attributes.class));
 const expectedOffset=vertical?35:45;
 assert.equal(after(),expectedOffset,kind+' reserves Hidden subtree size and padding');
 assert.equal(e("desired(widgets.get('root'))."+(vertical?'h':'w')),vertical?45:85);
 assert.equal(e("boxes.has('hiddenChild')"),true,'Hidden descendants participate in layout');
 assert.equal(e("boxes.has('collapsedChild')"),false,'Collapsed descendants have no layout');
 assert.deepEqual(painted(),['root','before','after'],'Hidden subtree has no shapes, outlines or hit targets');
 assert.equal(e("treeRows.get('child').row.classList.contains('not-rendered')"),true);
 v.ctx.select('child');assert.match(v.ids.summary.textContent,/hidden by ancestor/);
 assert.equal(selectionMarks().length,0,'Selecting a hidden descendant does not reveal it');
 v.ctx.select('panel');assert.equal(selectionMarks().length,0,'Hidden parent has no selection handle');
 v.ctx.setVisible('panel',true);assert.equal(after(),expectedOffset);
 assert.deepEqual(painted(),['root','before','panel','child','after']);
 assert.equal(e("treeRows.get('child').row.classList.contains('not-rendered')"),false);
 v.ctx.setVisible('hiddenChild',true);assert.ok(painted().includes('hiddenChild'));
 assert.equal(after(),expectedOffset,'Showing Hidden does not shift its siblings');
 v.ctx.setVisible('panel',false);assert.deepEqual(painted(),['root','before','after']);
 assert.equal(e("treeRows.get('hiddenChild').check.checked"),true,'Parent preserves child override');
 v.ctx.setVisible('panel',true);assert.ok(painted().includes('hiddenChild'));
 v.ids['reset-visibility'].onclick();assert.equal(after(),expectedOffset);
 assert.deepEqual(painted(),['root','before','after']);
 assert.equal(e("visibility(widgets.get('panel'))"),'Hidden','Reset restores exported Hidden');
 assert.equal(e('JSON.stringify(data)'),source,'Visibility controls do not change source');
 assert.equal(v.ctx.reviewNotes().changes.length,0,'Visibility is not an authored review change');

 fixture.widgets.find(w=>w.id==='panel').props.Visibility='Collapsed';
 const collapsed=createViewer(fixture),ce=collapsed.evaluate;
 assert.equal(ce(`boxes.get('after').${axis}`),vertical?8:10,kind+' excludes Collapsed subtree and padding');
 assert.equal(ce('boxes.size'),3);
 collapsed.ctx.setVisible('panel',true);assert.equal(ce(`boxes.get('after').${axis}`),expectedOffset);
 collapsed.ids['reset-visibility'].onclick();assert.equal(ce('boxes.size'),3);

 // An empty box has no synthetic size when all of its children are Collapsed.
 fixture.widgets.find(w=>w.id==='panel').props.Visibility='Hidden';
 for(const id of ['child','hiddenChild'])fixture.widgets.find(w=>w.id===id).props.Visibility='Collapsed';
 const empty=createViewer(fixture);
 assert.equal(empty.evaluate(`desired(widgets.get('panel')).${vertical?'h':'w'}`),0);
 assert.equal(empty.evaluate(`boxes.get('after').${axis}`),vertical?17:15,'Only the empty panel slot padding remains');
 fixture.widgets=fixture.widgets.filter(w=>!['before','after'].includes(w.id));
 fixture.widgets[0].kind='ScaleBox';fixture.widgets[0].children=['panel'];
 const scaledEmpty=createViewer(fixture);
 assert.equal(scaledEmpty.evaluate("Object.values(boxes.get('panel')).every(Number.isFinite)"),true,
  'An empty box inside ScaleBox has finite bounds');
}
(async()=>{
 await ids['copy-review'].onclick();assert.equal(fallbackSelected,true,'File-page clipboard fallback selects notes');
 let copied;ctx.navigator={clipboard:{writeText:async value=>{copied=value}}};
 await ids['copy-review'].onclick();assert.equal(JSON.parse(copied).changes.length,1);
 console.log('Widget viewer: Hidden/Collapsed nested layout, visibility controls, textures, local tint/opacity/text, drag coordinates, nested moves, cancel/reset, source isolation and clipboard passed.');
})().catch(error=>{console.error(error);process.exitCode=1});
