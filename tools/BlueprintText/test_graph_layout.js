// node test_graph_layout.js <generated HTML>; geometry and SVG update checks.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const context=vm.createContext({});
vm.runInContext(fs.readFileSync(__dirname+'/graph_layout.js','utf8'),context);
const page=fs.readFileSync(process.argv[2],'utf8');
const decode=s=>s.replace(/&quot;/g,'"').replace(/&#x27;/g,"'").replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&');
const layouts=[...page.matchAll(/<svg data-layout="([^"]+)"/g)].map(m=>JSON.parse(decode(m[1])));
assert.equal(layouts.length,4,'Both views and directions contain geometry');
function element(){return {attributes:{},setAttribute(k,v){this.attributes[k]=String(v)}}}
const branches={nodes:[['root',0],['aRight',1],['zLeft',1],['aChild',2],['zChild',2]].map(([id,rank])=>({id,rank,kind:id==='root'?'branch':'call',w:330,h:70})),
    edges:[['root','then','zLeft','exec',true],['root','else','aRight','exec',true],
           ['zLeft','then','aChild','exec',true],['aRight','then','zChild','exec',true],
           ['zLeft','result','zChild','value',false],['aRight','result','aChild','value',false]]};
for(const columns of [2,3,6,12]){
    const result=context.layoutRows(branches,columns),b=result.boxes;
    assert.ok(b.get('zLeft').x<b.get('aRight').x,'then branch precedes else regardless of IDs');
    assert.ok(b.get('aChild').x<b.get('zChild').x,'execution parents take priority over crossed data dependencies');
}
for(const data of layouts){
    const original=JSON.stringify(data),normal=context.layoutRows(data,3);
    // Default browser layout agrees with the standalone Python SVG layout.
    for(const n of data.nodes){const b=normal.boxes.get(n.id);assert.equal(b.x,n.x);assert.equal(b.y,n.y)}
    const nodes=data.nodes.map(n=>{const rect=element();return Object.assign(element(),{dataset:{nodeId:n.id},rect,querySelector:()=>rect})});
    const edges=data.edges.map(()=>{
        const parts=Object.fromEntries(['.edge-line','.edge-hit','.label'].map(k=>[k,element()]));
        return {parts,querySelector:s=>parts[s]};
    });
    const svg=Object.assign(element(),{dataset:{layout:original},querySelectorAll:s=>s==='.graph-node'?nodes:edges});
    for(const columns of [1,6,12,3]){
        context.applyRowLayout(svg,columns);
        const result=context.layoutRows(data,columns),rows=new Map();
        const incoming=new Map(),outgoing=new Map();
        result.edges.forEach((edge,i)=>{
            const points=edge.path.match(/-?\d+(?:\.\d+)?/g).map(Number),e=data.edges[i];
            for(const [ports,id,x] of [[outgoing,e[0],points[data.direction==='LR'?1:0]],[incoming,e[2],points.at(data.direction==='LR'?-1:-2)]]){
                if(!ports.has(id))ports.set(id,[[],[]]);ports.get(id)[Number(e[4])].push(x);
            }
        });
        for(const [values,actions] of [...incoming.values(),...outgoing.values()]){
            if(values.length&&actions.length)assert.ok(Math.max(...values)<Math.min(...actions),'all data ports precede execution ports');
        }
        for(const n of data.nodes){
            const b=result.boxes.get(n.id);rows.set(b.y,(rows.get(b.y)||0)+1);
            assert.ok(b.x>=0&&b.x+b.w<=result.width&&b.y+b.h<=result.height);
            assert.equal(nodes.find(v=>v.dataset.nodeId===n.id).attributes.transform,`translate(${b.x-n.x},${b.y-n.y})`);
        }
        if(data.direction!=='LR')assert.ok([...rows.values()].every(count=>count<=columns));
        edges.forEach((e,i)=>{
            assert.equal(e.parts['.edge-line'].attributes.d,result.edges[i].path);
            assert.equal(e.parts['.edge-hit'].attributes.d,result.edges[i].path);
            const label=result.edges[i].label;
            if(label)assert.equal(e.parts['.label'].attributes.x,String(label.x));
        });
        // The sequence ends back at the original layout, with no accumulated translation.
        if(columns===3)assert.ok(nodes.every(n=>n.attributes.transform==='translate(0,0)'));
    }
    context.applyRowLayout(svg,3,false);
    const compactData=JSON.parse(original);
    for(const n of compactData.nodes)n.h=n.compactH??n.h;
    const compact=context.layoutRows(compactData,3);
    for(const n of nodes)assert.equal(n.rect.attributes.height,String(compact.boxes.get(n.dataset.nodeId).h));
    assert.ok(compact.height<=normal.height);
    if(data.nodes.some(n=>n.compactH<n.h))assert.ok(compact.height<normal.height,'Hidden rows reduce graph height');
    edges.forEach((e,i)=>assert.equal(e.parts['.edge-line'].attributes.d,compact.edges[i].path));
    // Changing row capacity while hidden must keep reduced card heights.
    context.applyRowLayout(svg,6,false);
    for(const n of nodes)assert.equal(n.rect.attributes.height,String(compact.boxes.get(n.dataset.nodeId).h));
    context.applyRowLayout(svg,3,true);
    for(const n of nodes)assert.equal(n.rect.attributes.height,String(normal.boxes.get(n.dataset.nodeId).h));
    assert.ok(nodes.every(n=>n.attributes.transform==='translate(0,0)'));
    assert.equal(JSON.stringify(data),original);
}
console.log('Passed: browser/default SVG layout agreement, 1/6/12/3 row capacities, edge hit paths, labels and reversible SVG updates.');
