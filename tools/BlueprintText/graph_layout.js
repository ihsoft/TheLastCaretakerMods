// Geometry only: changing row capacity never rebuilds node content or source data.
function normalizeColumns(value){
    const number=Number(value);
    return Number.isFinite(number)&&String(value).trim()!==''?Math.max(1,Math.min(12,Math.round(number))):8;
}
function compareKeys(a,b){
    for(let i=0;i<a.length;i++){if(a[i]<b[i])return -1;if(a[i]>b[i])return 1}return 0;
}
function pinOrder(pin){
    return [({'then':0,'else':1}[pin]??2),pin.replace(/\d+/g,n=>n.padStart(12,'0'))];
}
function portFractions(nodes,edges){
    return [[0,1],[2,3]].map(([endpoint,pin])=>{
        const ports=new Map(nodes.map(n=>[n.id,new Map()]));
        for(const e of edges)ports.get(e[endpoint]).set(e[pin],[Number(e[4]),...pinOrder(e[pin])]);
        return new Map([...ports].map(([id,values])=>[id,new Map([...values].sort((a,b)=>compareKeys(a[1],b[1]))
            .map(([p],i)=>[p,(i+1)/(values.size+1)]))]));
    });
}
function segmentsCross(a,b,c,d){
    const turn=(p,q,r)=>(q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0]);
    return turn(a,b,c)*turn(a,b,d)<-1e-7&&turn(c,d,a)*turn(c,d,b)<-1e-7;
}
// Keep this geometry contract in sync with graph_layout.py (standalone SVG).
function fragmentBoxes(data,columns){
    const {nodes,edges}=data,horizontal=data.direction==='LR',by=new Map(nodes.map(n=>[n.id,n]));
    const incoming=new Map(nodes.map(n=>[n.id,[]])),outgoing=new Map(nodes.map(n=>[n.id,[]])),adjacent=new Map(nodes.map(n=>[n.id,[]]));
    edges.forEach((e,i)=>{outgoing.get(e[0]).push(i);incoming.get(e[2]).push(i);adjacent.get(e[0]).push(e[2]);adjacent.get(e[2]).push(e[0])});
    const [outputs,inputs]=portFractions(nodes,edges);
    const cross=n=>by.get(n)[horizontal?'h':'w'],flow=n=>by.get(n)[horizontal?'w':'h'];
    const delta=e=>cross(e[0])*outputs.get(e[0]).get(e[1])-cross(e[2])*inputs.get(e[2]).get(e[3]);
    const forward=i=>edges[i][4]&&by.get(edges[i][0]).rank<by.get(edges[i][2]).rank;
    const edgeKey=i=>[edges[i][0],...pinOrder(edges[i][1]),edges[i][2],edges[i][3]];
    const primary=map=>new Map([...map].map(([n,indices])=>[n,indices.filter(forward).sort((a,b)=>compareKeys(edgeKey(a),edgeKey(b)))[0]??null]));
    const primaryIn=primary(incoming),primaryOut=primary(outgoing),seen=new Set(),components=[];
    const executionNodes=new Set(nodes.filter(n=>[...incoming.get(n.id),...outgoing.get(n.id)].some(i=>edges[i][4])).map(n=>n.id));
    const dataIn=new Map();
    for(const n of by.keys()){
        const sources=new Set(incoming.get(n).map(i=>edges[i][0]).filter(s=>!executionNodes.has(s)));
        if(!executionNodes.has(n)&&sources.size===1&&incoming.get(n).every(i=>sources.has(edges[i][0]))){
            const source=[...sources][0],targets=new Set(outgoing.get(source).map(i=>edges[i][2]));
            if(by.get(source).rank<by.get(n).rank&&targets.size===1&&targets.has(n))
                dataIn.set(n,incoming.get(n).filter(i=>edges[i][0]===source).sort((a,b)=>compareKeys(edgeKey(a),edgeKey(b)))[0]);
        }
    }
    for(const root of [...by.keys()].sort()){
        if(seen.has(root))continue;
        const pending=[root],component=[];seen.add(root);
        while(pending.length){const n=pending.pop();component.push(n);for(const other of [...adjacent.get(n)].sort())if(!seen.has(other)){seen.add(other);pending.push(other)}}
        components.push(component);
    }
    const boxes=new Map(),rowWidth=columns*390-60;
    let shelfX=40,shelfY=240,shelfHeight=0;
    for(const component of components){
        const ordered=[...component].sort((a,b)=>compareKeys([by.get(a).rank,a],[by.get(b).rank,b]));
        const chainOf=new Map(),offsets=new Map(),chains=[];
        for(const n of ordered){
            const i=executionNodes.has(n)?primaryIn.get(n):(dataIn.get(n)??null);
            if(i!==null&&(!executionNodes.has(n)||primaryOut.get(edges[i][0])===i)){const source=edges[i][0];chainOf.set(n,chainOf.get(source));offsets.set(n,offsets.get(source)+delta(edges[i]))}
            else{chainOf.set(n,chains.length);offsets.set(n,0);chains.push([])}
            chains[chainOf.get(n)].push(n);
        }
        const isExec=chain=>executionNodes.has(chain[0]);
        const chainKey=chain=>{const execution=isExec(chain),root=chain[0],i=primaryIn.get(root);return [Number(!execution),execution?by.get(root).rank:-by.get(chain.at(-1)).rank,...(i===null?[root,-1,'','','']:edgeKey(i)),root]};
        let occupied=new Map();const placed=new Map();
        const execEdges=component.flatMap(n=>outgoing.get(n).filter(i=>edges[i][4]).map(i=>edges[i]));
        const orderedChains=[...chains].sort((a,b)=>compareKeys(chainKey(a),chainKey(b)));
        const schedule=[...orderedChains.map(chain=>[chain,false]),...[0,1].flatMap(()=>orderedChains.filter(chain=>!isExec(chain)).map(chain=>[chain,true]))];
        for(const [chain,refine] of schedule){
            const previous=refine?placed.get(chain[0])-offsets.get(chain[0]):null;
            if(refine){
                for(const n of chain)placed.delete(n);
                occupied=new Map();for(const [n,position] of placed){const rank=by.get(n).rank;if(!occupied.has(rank))occupied.set(rank,[]);occupied.get(rank).push([position,position+cross(n)])}
            }
            const incomingAnchors=[],outgoingAnchors=[];
            for(const n of chain){
                for(const i of incoming.get(n)){const e=edges[i];if(placed.has(e[0]))incomingAnchors.push([e[4],placed.get(e[0])+delta(e)-offsets.get(n)])}
                for(const i of outgoing.get(n)){const e=edges[i];if(placed.has(e[2]))outgoingAnchors.push([e[4],placed.get(e[2])-delta(e)-offsets.get(n)])}
            }
            const execution=isExec(chain),anchors=execution?(incomingAnchors.length?incomingAnchors:outgoingAnchors):(outgoingAnchors.length?outgoingAnchors:incomingAnchors);
            const executionAnchors=anchors.filter(a=>a[0]);
            const preferred=(executionAnchors.length?executionAnchors:anchors).map(a=>a[1]).sort((a,b)=>a-b);
            const desired=preferred.length?preferred[Math.floor((preferred.length-1)/2)]:0,forbidden=[];
            for(const n of chain)for(const [lo,hi] of occupied.get(by.get(n).rank)||[])forbidden.push([lo-offsets.get(n)-cross(n)-60,hi-offsets.get(n)+60]);
            const candidates=[desired,...forbidden.flat(),...(refine?[previous]:[]),...(execution?[]:[...incomingAnchors,...outgoingAnchors].map(a=>a[1]))].filter(v=>(!execution||v>=desired)&&forbidden.every(([lo,hi])=>v<=lo+1e-7||v>=hi-1e-7));
            const incident=[...new Set(chain.flatMap(n=>[...incoming.get(n),...outgoing.get(n)]).filter(i=>placed.has(edges[i][0])||placed.has(edges[i][2])))].sort((a,b)=>a-b);
            const cost=value=>{
                if(execution)return [Math.abs(value-desired),value];
                const position=n=>placed.has(n)?placed.get(n):value+offsets.get(n);
                const endpoints=e=>[[by.get(e[0]).rank,position(e[0])+cross(e[0])*outputs.get(e[0]).get(e[1])],
                    [by.get(e[2]).rank,position(e[2])+cross(e[2])*inputs.get(e[2]).get(e[3])]];
                let crossings=0;
                for(const i of incident){const e=edges[i];for(const spine of execEdges){
                    if([spine[0],spine[2]].includes(e[0])||[spine[0],spine[2]].includes(e[2]))continue;
                    if(segmentsCross(...endpoints(e),...endpoints(spine)))crossings++;
                }}
                const distance=values=>values.reduce((sum,[,v])=>sum+Math.abs(value-v),0)/Math.max(1,values.length);
                const score=(refine?[...incomingAnchors,...outgoingAnchors].reduce((sum,[,v])=>sum+Math.abs(value-v),0):2*distance(outgoingAnchors)+distance(incomingAnchors))+390*crossings;
                return [Math.round(score*1e6)/1e6,Math.abs(value-(refine?previous:desired)),value];
            };
            const costs=new Map(candidates.map(v=>[v,cost(v)]));
            candidates.sort((a,b)=>compareKeys(costs.get(a),costs.get(b)));
            const shift=candidates[0];
            for(const n of chain){const rank=by.get(n).rank;placed.set(n,shift+offsets.get(n));if(!occupied.has(rank))occupied.set(rank,[]);occupied.get(rank).push([placed.get(n),placed.get(n)+cross(n)])}
        }
        const main=new Map();let cursor=0;
        for(const rank of [...new Set(component.map(n=>by.get(n).rank))].sort((a,b)=>a-b)){main.set(rank,cursor);cursor+=Math.max(...component.filter(n=>by.get(n).rank===rank).map(flow))+(horizontal?170:110)}
        const minimum=Math.min(...placed.values());
        const local=new Map(component.map(n=>[n,horizontal?[main.get(by.get(n).rank),placed.get(n)-minimum]:[placed.get(n)-minimum,main.get(by.get(n).rank)]]));
        const margin=component.some(n=>outgoing.get(n).some(i=>by.get(edges[i][0]).rank>=by.get(edges[i][2]).rank))?80:0;
        const width=Math.max(...component.map(n=>local.get(n)[0]+by.get(n).w))+2*margin,height=Math.max(...component.map(n=>local.get(n)[1]+by.get(n).h))+2*margin;
        if(shelfX>40&&shelfX-40+width>rowWidth){shelfX=40;shelfY+=shelfHeight+110;shelfHeight=0}
        for(const [n,[x,y]] of local)boxes.set(n,{x:x+shelfX+margin,y:y+shelfY+margin,w:by.get(n).w,h:by.get(n).h,kind:by.get(n).kind});
        shelfX+=width+60;shelfHeight=Math.max(shelfHeight,height);
    }
    return boxes;
}
// Execution-only card-aware routing, mirrored in graph_routing.py.
function segmentClear(a,b,rects){
    return !rects.some(([l,t,r,d])=>a[0]===b[0]
        ?l<a[0]&&a[0]<r&&Math.max(Math.min(a[1],b[1]),t)<Math.min(Math.max(a[1],b[1]),d)
        :t<a[1]&&a[1]<d&&Math.max(Math.min(a[0],b[0]),l)<Math.min(Math.max(a[0],b[0]),r));
}
function curveHits(points,rect){
    const [l,t,r,b]=rect,xs=points.map(p=>p[0]),ys=points.map(p=>p[1]);
    if(Math.max(...xs)<=l||Math.min(...xs)>=r||Math.max(...ys)<=t||Math.min(...ys)>=b)return false;
    if(Math.max(Math.max(...xs)-Math.min(...xs),Math.max(...ys)-Math.min(...ys))<.5)return true;
    const [a,d,c,e]=points,mid=(p,q)=>[(p[0]+q[0])/2,(p[1]+q[1])/2];
    const ad=mid(a,d),dc=mid(d,c),ce=mid(c,e),adc=mid(ad,dc),dce=mid(dc,ce),center=mid(adc,dce);
    return curveHits([a,ad,adc,center],rect)||curveHits([center,dce,ce,e],rect);
}
function simplifyRoute(points){
    const result=[];
    for(const p of points){
        if(result.length&&result.at(-1)[0]===p[0]&&result.at(-1)[1]===p[1])continue;
        while(result.length>=2&&((result.at(-2)[0]===result.at(-1)[0]&&result.at(-1)[0]===p[0])||
            (result.at(-2)[1]===result.at(-1)[1]&&result.at(-1)[1]===p[1])))result.pop();
        result.push(p);
    }
    return result;
}
function orthogonalRoute(start,end,rects){
    const xs=[...new Set([start[0],end[0],...rects.flatMap(r=>[r[0],r[2]])])].sort((a,b)=>a-b);
    const ys=[...new Set([start[1],end[1],...rects.flatMap(r=>[r[1],r[3]])])].sort((a,b)=>a-b);
    const length=(a,b)=>Math.abs(a[0]-b[0])+Math.abs(a[1]-b[1]);
    const candidates=[[start,[start[0],end[1]],end],[start,[end[0],start[1]],end],
        ...ys.map(y=>[start,[start[0],y],[end[0],y],end]),...xs.map(x=>[start,[x,start[1]],[x,end[1]],end])].map(simplifyRoute);
    const metric=value=>Math.round(value*1e6)/1e6;
    const score=p=>metric(p.slice(1).reduce((total,b,i)=>total+length(p[i],b),0)+18*(p.length-2));
    candidates.sort((a,b)=>score(a)-score(b));
    for(const p of candidates)if(p.slice(1).every((b,i)=>segmentClear(p[i],b,rects)))return p;
    // Binary heap with insertion-order tie breaking matches Python's A* queue.
    const heap=[],less=(a,b)=>a.priority<b.priority||(a.priority===b.priority&&a.serial<b.serial);
    function push(item){heap.push(item);let i=heap.length-1;while(i){const p=(i-1)>>1;if(!less(heap[i],heap[p]))break;[heap[i],heap[p]]=[heap[p],heap[i]];i=p}}
    function pop(){const item=heap[0],last=heap.pop();if(heap.length){heap[0]=last;let i=0;while(true){let c=i*2+1;if(c>=heap.length)break;if(c+1<heap.length&&less(heap[c+1],heap[c]))c++;if(!less(heap[c],heap[i]))break;[heap[i],heap[c]]=[heap[c],heap[i]];i=c}}return item}
    const origin=[xs.indexOf(start[0]),ys.indexOf(start[1]),-1],target=[xs.indexOf(end[0]),ys.indexOf(end[1])];
    const key=s=>s.join(','),costs=new Map([[key(origin),0]]),parents=new Map();let serial=0;
    push({priority:0,serial:serial++,cost:0,state:origin});
    while(heap.length){
        const {cost,state}=pop();if(cost!==costs.get(key(state)))continue;
        const [x,y,direction]=state;
        if(x===target[0]&&y===target[1]){const p=[];let s=state;while(s){p.push([xs[s[0]],ys[s[1]]]);s=parents.get(key(s))}return simplifyRoute(p.reverse())}
        for(const [nx,ny,axis] of [[x+1,y,0],[x,y+1,1],[x-1,y,0],[x,y-1,1]]){
            if(nx<0||nx>=xs.length||ny<0||ny>=ys.length)continue;
            const a=[xs[x],ys[y]],b=[xs[nx],ys[ny]];if(!segmentClear(a,b,rects))continue;
            const value=metric(cost+length(a,b)+(direction===-1||direction===axis?0:18)),other=[nx,ny,axis];
            if(value<(costs.get(key(other))??Infinity)){costs.set(key(other),value);parents.set(key(other),state);push({priority:metric(value+length(b,end)),serial:serial++,cost:value,state:other})}
        }
    }
    throw new Error('No free corridor between graph ports');
}
function roundedPath(points){
    let path=`M${points[0][0]},${points[0][1]}`;
    for(let i=1;i<points.length-1;i++){
        const [a,b,c]=points.slice(i-1,i+2),incoming=Math.abs(a[0]-b[0])+Math.abs(a[1]-b[1]),outgoing=Math.abs(b[0]-c[0])+Math.abs(b[1]-c[1]);
        const radius=Math.min(6,incoming/2,outgoing/2);
        const p=[b[0]+(a[0]-b[0])*radius/incoming,b[1]+(a[1]-b[1])*radius/incoming];
        const q=[b[0]+(c[0]-b[0])*radius/outgoing,b[1]+(c[1]-b[1])*radius/outgoing];
        path+=` L${p[0]},${p[1]} Q${b[0]},${b[1]} ${q[0]},${q[1]}`;
    }
    return path+` L${points.at(-1)[0]},${points.at(-1)[1]}`;
}
function edgePath(start,end,boxes,source,target,horizontal,execution){
    const [sx,sy]=start,[ex,ey]=end,bend=Math.max(35,(horizontal?ex-sx:ey-sy)/2);
    const controls=horizontal?[start,[sx+bend,sy],[ex-bend,ey],end]:[start,[sx,sy+bend],[ex,ey-bend],end];
    if(!execution){
        if(source===target){const b=boxes.get(source);controls[1]=horizontal?[sx+40,b.y+b.h+55]:[b.x+b.w+55,sy+40];controls[2]=horizontal?[ex-40,b.y+b.h+55]:[b.x+b.w+55,ey-40]}
        return `M${sx},${sy} C${controls[1][0]},${controls[1][1]} ${controls[2][0]},${controls[2][1]} ${ex},${ey}`;
    }
    const raw=new Map([...boxes].map(([n,b])=>[n,[b.x,b.y,b.x+b.w,b.y+b.h]]));
    const rects=new Map([...raw].map(([n,[l,t,r,b]])=>[n,[l-14,t-14,r+14,b+14]]));
    if(source!==target&&![...rects].some(([n,r])=>curveHits(controls,n===source||n===target?raw.get(n):r)))
        return `M${sx},${sy} C${controls[1][0]},${controls[1][1]} ${controls[2][0]},${controls[2][1]} ${ex},${ey}`;
    const leave=horizontal?[sx+24,sy]:[sx,sy+24],enter=horizontal?[ex-24,ey]:[ex,ey-24];
    return roundedPath([start,...orthogonalRoute(leave,enter,[...rects.values()]),end]);
}
function layoutRows(data, columns) {
    columns=normalizeColumns(columns);
    const horizontal=data.direction==='LR';
    const boxes=fragmentBoxes(data,columns),[outputFractions,inputFractions]=portFractions(data.nodes,data.edges);
    const edges=data.edges.map((e,i)=>{
        const [source,output,target,,execution]=e,a=boxes.get(source),b=boxes.get(target);
        const sourceFraction=outputFractions.get(source).get(output),targetFraction=inputFractions.get(target).get(e[3]);
        const sx=horizontal?a.x+a.w:a.x+a.w*sourceFraction,
              sy=horizontal?a.y+a.h*sourceFraction:a.y+a.h;
        const ex=horizontal?b.x:b.x+b.w*targetFraction,
              ey=horizontal?b.y+b.h*targetFraction:b.y;
        const path=edgePath([sx,sy],[ex,ey],boxes,source,target,horizontal,execution);
        return {path,label:execution?(horizontal?{x:sx+10,y:sy-7}:{x:sx,y:sy+13}):null};
    });
    return {columns,boxes,edges,width:Math.max(950,data.minWidth||0,...[...boxes.values()].map(b=>b.x+b.w+80)),
            height:Math.max(240,...[...boxes.values()].map(b=>b.y+b.h))+90};
}

function applyRowLayout(svg, columns, showUnconnected=true) {
    if(!svg.dataset.layout)return;
    const data=JSON.parse(svg.dataset.layout);
    if(!showUnconnected)for(const n of data.nodes)n.h=n.compactH??n.h;
    const result=layoutRows(data,columns);
    const nodes=new Map([...svg.querySelectorAll('.graph-node')].map(n=>[n.dataset.nodeId,n]));
    for(const n of data.nodes){
        const b=result.boxes.get(n.id);
        nodes.get(n.id).setAttribute('transform',`translate(${b.x-n.x},${b.y-n.y})`);
        nodes.get(n.id).querySelector('rect').setAttribute('height',b.h);
        const frame=nodes.get(n.id).querySelector('.nested-graph-frame');
        if(frame)frame.setAttribute('height',b.h-8);
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
