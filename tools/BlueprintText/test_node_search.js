// Synthetic DOM contract: root search, duplicate IDs, safe text and Back restoration.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const code=fs.readFileSync(__dirname+'/node_search.js','utf8');
function run(records,url='file:///preview/index.html'){
    const events={},elements={};let focused=null,navigated=null;
    function element(tag='div'){
        return {tag,children:[],handlers:{},value:'',checked:false,hidden:false,textContent:'',
            append(...items){this.children.push(...items)},replaceChildren(...items){this.children=items},
            addEventListener(name,fn){this.handlers[name]=fn},focus(){focused=this},
            querySelector(tag){for(const child of this.children){if(child.tag===tag)return child;const found=child.querySelector(tag);if(found)return found}return null}};
    }
    for(const id of ['data','query','before','before-label','results','status','clear'])elements['node-search-'+id]=element();
    elements['node-search-data'].textContent=JSON.stringify(records);
    const location={href:url,assign(href){navigated=href}};
    const history={state:{keep:true},replaceState(state,unused,href){this.state=state;location.href=href}};
    vm.runInNewContext(code,{document:{getElementById:id=>elements[id],createElement:element},
        window:{addEventListener:(name,fn)=>events[name]=fn},location,history,URL});
    const q=elements['node-search-query'],before=elements['node-search-before'],results=elements['node-search-results'];
    return {elements,location,history,events,q,before,results,
        search(value){q.value=value;q.handlers.input()},key(key){q.handlers.keydown({key,preventDefault(){}})},
        get focused(){return focused},get navigated(){return navigated}};
}
const records=[
    {id:'K2Node_CallFunction_78',title:'Break Hit Result',path:'ModActor / EventGraph / Hold',asset:'/Game/A',snapshot:'Current',href:'a.html#node-K2Node_CallFunction_78'},
    {id:'K2Node_CallFunction_78',title:'Other',path:'PitchInputActor / Tick',asset:'/Game/B',snapshot:'Current',href:'b.html#node-K2Node_CallFunction_78'},
    {id:'K2Node_CallFunction_78',title:'Old',path:'ModActor / Old',asset:'/Game/A',snapshot:'Before',href:'a-before.html#node-K2Node_CallFunction_78'},
    {id:'unique',title:'<img src=x onerror=alert(1)>',path:'ModActor / New',asset:'/Game/A',snapshot:'Current',href:'a.html#node-unique'}];
const app=run(records);
assert.equal(app.results.children.length,0);
app.search('k2node_callfunction_78');assert.equal(app.results.children.length,2);
app.key('Enter');assert.equal(app.navigated,null);assert.equal(app.focused,app.results.children[0].children[0]);
app.before.checked=true;app.before.handlers.change();assert.equal(app.results.children.length,3);
assert.equal(new URL(app.location.href).searchParams.get('before'),'1');
app.search('break hit');assert.equal(app.results.children.length,1);app.key('Enter');assert.equal(app.navigated,records[0].href);
app.search('78 pitchinputactor');assert.equal(app.results.children[0].children[0].href,records[1].href);
const saved=app.location.href;app.q.value='lost';app.before.checked=false;app.events.pageshow();
assert.equal(app.q.value,'78 pitchinputactor');assert.equal(app.before.checked,true);assert.equal(app.location.href,saved);
app.search('unique');assert.equal(app.results.children[0].children[0].children[1].textContent,records[3].title);
app.search('no matches');assert.equal(app.results.children.length,0);assert.equal(app.elements['node-search-status'].textContent,'No matching nodes.');
app.key('Escape');assert.equal(app.q.value,'');assert.equal(new URL(app.location.href).searchParams.has('q'),false);assert.deepEqual(app.history.state,{keep:true});
const currentOnly=run(records.filter(n=>n.snapshot==='Current'),'file:///preview/index.html?q=unique&before=1');
assert.equal(currentOnly.elements['node-search-before-label'].hidden,true);assert.equal(currentOnly.before.checked,false);assert.equal(currentOnly.results.children.length,1);
const many=run(Array.from({length:105},(_,i)=>({...records[0],id:'node'+i})));many.search('node');
assert.equal(many.results.children.length,100);assert.match(many.elements['node-search-status'].textContent,/105 matches/);
console.log('Passed: root node search, duplicate scopes, snapshots, keyboard, safe text and history restoration.');
