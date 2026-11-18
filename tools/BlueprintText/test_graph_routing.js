// Independent path sampling and Python/browser routing parity on generated HTML.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const context=vm.createContext({});vm.runInContext(fs.readFileSync(__dirname+'/graph_layout.js','utf8'),context);
const decode=s=>s.replace(/&quot;/g,'"').replace(/&#x27;/g,"'").replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&');
const pockets=[[-20,-20,-10,20],[-10,-20,40,-10],[-10,10,40,20],
    [110,-20,120,20],[60,-20,110,-10],[60,10,110,20],[45,-30,55,30]];
const pocketRoute=context.orthogonalRoute([0,0],[100,0],pockets);
assert.ok(pocketRoute.length>=6,'Grid fallback handles opposing pockets');
assert.deepEqual(hits(context.roundedPath(pocketRoute),new Map(pockets.map(([l,t,r,b],i)=>[String(i),{x:l+2,y:t+2,w:r-l-4,h:b-t-4}]))),[]);
function samples(path){
    const tokens=path.match(/[MLCQ]|-?\d+(?:\.\d+)?(?:e[+-]?\d+)?/gi),points=[];let cursor=0,point=[0,0];
    while(cursor<tokens.length){
        const command=tokens[cursor++],count={M:1,L:1,Q:2,C:3}[command],controls=[point];
        for(let i=0;i<count;i++)controls.push([Number(tokens[cursor++]),Number(tokens[cursor++])]);
        if(command!=='M'){
            const length=controls.slice(1).reduce((total,b,i)=>total+Math.hypot(b[0]-controls[i][0],b[1]-controls[i][1]),0),steps=Math.max(1,Math.ceil(length/3));
            for(let i=0;i<=steps;i++){
                const t=i/steps;let p=controls;
                while(p.length>1)p=p.slice(1).map((b,j)=>[(1-t)*p[j][0]+t*b[0],(1-t)*p[j][1]+t*b[1]]);
                points.push(p[0]);
            }
        }
        point=controls.at(-1);
    }
    return points;
}
function hits(path,boxes){
    const points=samples(path),xs=points.map(p=>p[0]),ys=points.map(p=>p[1]);
    const l=Math.min(...xs),r=Math.max(...xs),t=Math.min(...ys),d=Math.max(...ys);
    return [...boxes].filter(([id,b])=>b.x<r&&b.x+b.w>l&&b.y<d&&b.y+b.h>t&&
        points.some(([x,y])=>b.x+1e-6<x&&x<b.x+b.w-1e-6&&b.y+1e-6<y&&y<b.y+b.h-1e-6)).map(([id])=>id);
}
function auditPage(file,widths=[1,8,12]){
    const page=fs.readFileSync(file,'utf8'),initial=Number(page.match(/id="nodes-per-row"[^>]*value="(\d+)"/)[1]);
    const sections=[...page.matchAll(/<section id="([^"]+)"[^>]*>([\s\S]*?)<\/section>/g)],results=[];
    for(const [,id,section] of sections){
        const data=JSON.parse(decode(section.match(/data-layout="([^"]+)"/)[1]));
        const original=JSON.stringify(data),staticPaths=[...section.matchAll(/class="edge-line" d="([^"]+)"/g)].map(m=>m[1]);
        const normal=context.layoutRows(data,initial);
        normal.edges.forEach((edge,i)=>{
            assert.equal(edge.path.replace(/-?\d+(?:\.\d+)?(?:e[+-]?\d+)?/gi,'#'),staticPaths[i].replace(/-?\d+(?:\.\d+)?(?:e[+-]?\d+)?/gi,'#'),id+' routing commands');
            const numbers=p=>p.match(/-?\d+(?:\.\d+)?(?:e[+-]?\d+)?/gi).map(Number),a=numbers(edge.path),b=numbers(staticPaths[i]);
            a.forEach((v,j)=>assert.ok(Math.abs(v-b[j])<1e-6,`${file} ${id} edge ${i} Python/JS route parity: ${edge.path} != ${staticPaths[i]}`));
        });
        for(const compact of [false,true])for(const width of widths){
            const input=structuredClone(data);if(compact)input.nodes.forEach(n=>n.h=n.compactH??n.h);
            const layout=context.layoutRows(input,width);let routed=0;
            layout.edges.forEach((edge,i)=>{
                if(data.edges[i][4]){
                    const collisions=hits(edge.path,layout.boxes);
                    assert.deepEqual(collisions,[],`${file} ${id} width=${width} compact=${compact}: ${JSON.stringify(data.edges[i])} intersects ${collisions}`);
                }else{
                    assert.match(edge.path,/^M[^MLCQ]+ C[^MLCQ]+$/,'Data links retain a single cubic curve');
                }
                if(edge.path.includes(' Q'))routed++;
            });
            if(width===initial&&compact)results.push({id,nodes:input.nodes.length,edges:layout.edges.length,routed});
        }
        assert.equal(JSON.stringify(data),original,'Source geometry remains unchanged');
    }
    return results;
}
if(require.main===module){console.log(JSON.stringify(auditPage(process.argv[2])));console.log('Passed: execution/card clearance, unchanged data curves, both directions, compact/expanded cards, adjustable width and Python/browser routing parity.');}
module.exports={auditPage,hits,samples,context};
