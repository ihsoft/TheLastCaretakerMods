// Geometry only: changing row capacity never rebuilds node content or source data.
function normalizeColumns(value){
    const number=Number(value);
    return Number.isFinite(number)&&String(value).trim()!==''?Math.max(1,Math.min(12,Math.round(number))):3;
}
function compareKeys(a,b){
    for(let i=0;i<a.length;i++){if(a[i]<b[i])return -1;if(a[i]>b[i])return 1}return 0;
}
function pinOrder(pin){
    return [({'then':0,'else':1}[pin]??2),pin.replace(/\d+/g,n=>n.padStart(12,'0'))];
}
function layoutRows(data, columns) {
    columns=normalizeColumns(columns);
    const groups=new Map(),boxes=new Map();
    for(const n of data.nodes){if(!groups.has(n.rank))groups.set(n.rank,[]);groups.get(n.rank).push(n)}
    const capacity=Math.min(columns,Math.max(1,...[...groups.values()].map(g=>g.length)));
    const parents=new Map(data.nodes.map(n=>[n.id,[]]));
    for(const edge of data.edges)parents.get(edge[2]).push(edge);
    function parentOrder(node){
        const all=parents.get(node.id),execution=all.filter(e=>e[4]);
        const available=(execution.length?execution:all).filter(e=>boxes.has(e[0]));
        if(!available.length)return [Infinity,2,'',node.id];
        const center=available.reduce((sum,e)=>{const b=boxes.get(e[0]);return sum+b.x+b.w/2},0)/available.length;
        const pin=available.map(e=>pinOrder(e[1])).sort(compareKeys)[0];
        return [center,...pin,node.id];
    }
    let y=210;
    for(const [,members] of [...groups].sort((a,b)=>a[0]-b[0])){
        members.sort((a,b)=>compareKeys(parentOrder(a),parentOrder(b)));
        for(let start=0;start<members.length;start+=columns){
            const row=members.slice(start,start+columns),offset=(capacity-row.length)*190;
            row.forEach((n,i)=>boxes.set(n.id,{x:40+offset+i*380,y,w:n.w,h:n.h,kind:n.kind}));
            y+=Math.max(...row.map(n=>n.h))+110;
        }
    }
    const incoming=new Map(data.nodes.map(n=>[n.id,[]])),outgoing=new Map(data.nodes.map(n=>[n.id,[]]));
    data.edges.forEach((e,i)=>{outgoing.get(e[0]).push(i);incoming.get(e[2]).push(i)});
    const portKey=(i,input)=>{const e=data.edges[i];return [Number(e[4]),...pinOrder(e[input?3:1]),e[input?0:2],e[input?1:3],i]};
    for(const indices of outgoing.values())indices.sort((a,b)=>compareKeys(portKey(a,false),portKey(b,false)));
    for(const indices of incoming.values())indices.sort((a,b)=>compareKeys(portKey(a,true),portKey(b,true)));
    const edges=data.edges.map((e,i)=>{
        const [source,output,target,,execution]=e,a=boxes.get(source),b=boxes.get(target);
        const outs=outgoing.get(source),ins=incoming.get(target);
        const sx=a.x+a.w*(outs.indexOf(i)+1)/(outs.length+1),sy=a.y+a.h;
        const ex=b.x+b.w*(ins.indexOf(i)+1)/(ins.length+1),ey=b.y;
        const bend=Math.max(35,(ey-sy)/2);
        let path=`M${sx},${sy} C${sx},${sy+bend} ${ex},${ey-bend} ${ex},${ey}`;
        if(source===target){const side=a.x+a.w+55;path=`M${sx},${sy} C${side},${sy+40} ${side},${ey-40} ${ex},${ey}`}
        return {path,label:execution&&a.kind==='branch'&&['then','else'].includes(output)?{x:sx,y:sy+13}:null};
    });
    return {columns,boxes,edges,width:Math.max(950,...[...boxes.values()].map(b=>b.x+b.w+80)),
            height:Math.max(210,...[...boxes.values()].map(b=>b.y+b.h))+90};
}

function applyRowLayout(svg, columns) {
    if(!svg.dataset.layout)return; // Horizontal layout retains its own geometry.
    const data=JSON.parse(svg.dataset.layout),result=layoutRows(data,columns);
    const nodes=new Map([...svg.querySelectorAll('.graph-node')].map(n=>[n.dataset.nodeId,n]));
    for(const n of data.nodes){
        const b=result.boxes.get(n.id);
        nodes.get(n.id).setAttribute('transform',`translate(${b.x-n.x},${b.y-n.y})`);
    }
    svg.querySelectorAll('.graph-edge').forEach((edge,i)=>{
        const geometry=result.edges[i];
        edge.querySelector('.edge-line').setAttribute('d',geometry.path);
        edge.querySelector('.edge-hit').setAttribute('d',geometry.path);
        const label=edge.querySelector('.label');
        if(label&&geometry.label){label.setAttribute('x',geometry.label.x);label.setAttribute('y',geometry.label.y)}
    });
    svg.setAttribute('viewBox',`0 0 ${result.width} ${result.height}`);
}
