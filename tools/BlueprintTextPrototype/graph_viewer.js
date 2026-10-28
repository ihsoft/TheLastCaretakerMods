// Embedded in generated HTML; no external scripts or network dependencies.
const box=document.querySelector('main'),view=document.getElementById('view'),direction=document.getElementById('direction');
const tip=document.getElementById('node-details'),detailText=document.getElementById('detail-text');
const detailIds=document.getElementById('detail-ids'),status=document.getElementById('copy-status');
const edgeTip=document.getElementById('edge-tooltip');
const detailVersion=document.getElementById('detail-version');
const rowCapacity=document.getElementById('nodes-per-row');
const showUnconnected=document.getElementById('unconnected');
function changeUnconnected(){
    const selected=graphIndex(svg).selected;
    closeDetails();svg.classList.toggle('hide-unconnected',!showUnconnected.checked);
    applyRowLayout(svg,rowCapacity.value,showUnconnected.checked);
    if(selected){
        show(); // Keep the user's zoom while centering the node in the new layout.
        const node=selected.getBoundingClientRect(),viewport=box.getBoundingClientRect();
        box.scrollLeft=Math.max(0,box.scrollLeft+(node.left+node.right)/2-viewport.left-box.clientWidth/2);
        box.scrollTop=Math.max(0,box.scrollTop+(node.top+node.bottom)/2-viewport.top-box.clientHeight/2);
    }else{box.scrollTop=0;box.scrollLeft=0;fit()}
}
let detailCopyText='';
let svg,scale=1,selectedId=null,detailNode=null,detailEdge=null,detailEdgeTitle=null;
const details=new WeakMap();
function closeEdgeDetails(){
    if(detailEdgeTitle)detailEdgeTitle.textContent=edgeTip.textContent;
    edgeTip.hidden=true;detailEdge?.classList.toggle('inspected',false);detailEdge=null;detailEdgeTitle=null;
}
function closeDetails(){tip.hidden=true;detailNode=null;closeEdgeDetails()}
function showDetails(node){
    closeEdgeDetails();
    detailNode=node;
    const data=JSON.parse(details.get(node));
    detailVersion.hidden=!['added','changed','removed'].includes(data.Status);
    detailVersion.value=data.Status==='removed'?'Before':'After';
    renderDetailContents();
    detailIds.textContent=JSON.parse(node.dataset.nodeIds).join(' → ');
    tip.hidden=false;
    const r=node.getBoundingClientRect(),container=box.getBoundingClientRect();
    // The panel scrolls with the graph and never flips above/over the source node.
    tip.style.left=(r.left-container.left+box.scrollLeft)+'px';
    tip.style.top=(r.bottom-container.top+box.scrollTop+6)+'px';
}
function renderDetailContents(){
    if(!detailNode)return;
    const raw=details.get(detailNode),data=JSON.parse(raw);
    const snapshot=detailVersion.hidden?data:data[detailVersion.value];
    detailCopyText=detailVersion.hidden?raw:JSON.stringify(snapshot,null,2);
    if(!detailVersion.hidden&&detailVersion.value==='After'&&detailNode.dataset.reviewDetails){
        // This markup is generated with escaped text by the offline renderer.
        detailText.innerHTML=detailNode.dataset.reviewDetails;
    }else detailText.textContent=detailCopyText;
}
detailVersion.addEventListener('change',renderDetailContents);
function toggleDetails(node){if(detailNode===node&&!tip.hidden)closeDetails();else showDetails(node)}
function showEdgeDetails(edge,event){
    if(detailEdge===edge&&!edgeTip.hidden){closeEdgeDetails();return}
    closeDetails();detailEdge=edge;edge.classList.toggle('inspected',true);
    detailEdgeTitle=edge.querySelector('title');edgeTip.textContent=detailEdgeTitle.textContent;
    detailEdgeTitle.textContent=''; // Avoid a delayed native tooltip over the click-open panel.
    edgeTip.hidden=false;
    edgeTip.style.left=Math.max(8,Math.min(event.clientX+12,innerWidth-edgeTip.offsetWidth-8))+'px';
    edgeTip.style.top=Math.max(8,Math.min(event.clientY+12,innerHeight-edgeTip.offsetHeight-8))+'px';
}
async function copyText(text,message){
    try{
        if(!navigator.clipboard?.writeText)throw new Error('Clipboard API unavailable');
        await navigator.clipboard.writeText(text);
        status.textContent=message;return true;
    }catch(error){
        const previous=document.activeElement,area=document.createElement('textarea');
        area.value=text;area.style.position='fixed';area.style.opacity='0';
        document.body.appendChild(area);area.select();
        let copied=false;
        try{copied=document.execCommand('copy')}catch(error){}finally{area.remove();previous?.focus?.()}
        status.textContent=copied?message:'Copy unavailable. Open Details, select text and copy manually.';
        return copied;
    }
}
async function copyNodeIds(node){
    const ids=JSON.parse(node.dataset.nodeIds);
    await copyText(ids.join('\n'),ids.length===1?'Node ID copied':ids.length+' node IDs copied');
}
function show(){const v=svg.viewBox.baseVal;svg.style.width=v.width*scale+'px';svg.style.height=v.height*scale+'px'}
function zoom(f){closeDetails();scale=Math.max(.05,Math.min(4,scale*f));show()}
function fit(all=false){closeDetails();const v=svg.viewBox.baseVal;scale=Math.min(1,Math.max(1,box.clientWidth-20)/v.width,all?box.clientHeight/v.height:1);show()}
const graphIndexes=new WeakMap();
function graphIndex(graph){
    if(graphIndexes.has(graph))return graphIndexes.get(graph);
    const nodes=new Map([...graph.querySelectorAll('.graph-node')].map(node=>
        [node.dataset.nodeId,{node,incoming:[],outgoing:[],neighbors:new Set()}]));
    for(const edge of graph.querySelectorAll('.graph-edge')){
        const from=nodes.get(edge.dataset.source),to=nodes.get(edge.dataset.target);
        from.outgoing.push(edge);to.incoming.push(edge);
        if(from!==to){from.neighbors.add(to.node);to.neighbors.add(from.node)}
    }
    const index={nodes,selected:null,edges:new Set(),neighbors:new Set()};
    graphIndexes.set(graph,index);return index;
}
function highlight(id){
    const state=graphIndex(svg);
    if(state.selected){state.selected.classList.toggle('selected',false);state.selected.setAttribute('aria-pressed','false')}
    for(const edge of state.edges)edge.classList.toggle('related',false);
    for(const node of state.neighbors)node.classList.toggle('neighbor',false);
    state.edges.clear();state.neighbors=new Set();state.selected=null;
    const entry=state.nodes.get(id);
    selectedId=entry?id:null;
    svg.classList.toggle('has-selection',!!entry);
    if(!entry)return;
    state.selected=entry.node;state.neighbors=entry.neighbors;
    state.edges=new Set([...entry.incoming,...entry.outgoing]);
    entry.node.classList.toggle('selected',true);entry.node.setAttribute('aria-pressed','true');
    for(const edge of state.edges)edge.classList.toggle('related',true);
    for(const node of state.neighbors)node.classList.toggle('neighbor',true);
}
function toggleNode(node){highlight(selectedId===node.dataset.nodeId?null:node.dataset.nodeId)}
box.addEventListener('click',event=>{
    if(tip.contains(event.target))return;
    const edge=event.target.closest('.graph-edge');
    if(edge&&svg.contains(edge)){event.stopPropagation();showEdgeDetails(edge,event);return}
    closeEdgeDetails();
    const node=event.target.closest('.graph-node');
    if(event.target.closest('.copy-node-id')&&node){event.stopPropagation();copyNodeIds(node);return}
    if(event.target.closest('.node-info')&&node){event.stopPropagation();toggleDetails(node);return}
    if(node&&svg.contains(node)){toggleNode(node)}else{highlight(null);closeDetails()}
});
window.addEventListener('resize',closeDetails);
box.addEventListener('scroll',closeEdgeDetails);
document.getElementById('copy-detail-ids').addEventListener('click',()=>{if(detailNode)copyNodeIds(detailNode)});
document.getElementById('copy-details').addEventListener('click',()=>copyText(detailCopyText,'Node details copied'));
document.getElementById('close-details').addEventListener('click',closeDetails);
document.addEventListener('keydown',event=>{
    if(event.key==='Escape'){highlight(null);closeDetails();return}
    const node=event.target.closest('.graph-node');
    if(node&&svg.contains(node)&&(event.key==='Enter'||event.key===' ')){
        event.preventDefault();
        if(event.target.closest('.copy-node-id'))copyNodeIds(node);
        else if(event.target.closest('.node-info'))toggleDetails(node);
        else toggleNode(node);
    }
});
document.querySelectorAll('.graph-node').forEach(node=>{
    const title=node.querySelector('title');details.set(node,title.textContent);title.remove();
    node.setAttribute('tabindex','0');node.setAttribute('role','button');
    node.setAttribute('aria-pressed','false');
    node.setAttribute('aria-label','Highlight connections for '+JSON.parse(node.dataset.nodeIds).join(', '));
});
function choose(){
    if(svg)highlight(null);
    closeDetails();const id=view.value+'-'+direction.value;
    document.querySelectorAll('section').forEach(s=>s.hidden=s.id!==id);
    svg=document.getElementById(id).querySelector('svg');
    rowCapacity.disabled=direction.value!=='TB';
    changeUnconnected();
    highlight(null);box.scrollTop=0;box.scrollLeft=0;fit();
}
function changeColumns(){
    rowCapacity.value=String(normalizeColumns(rowCapacity.value));
    closeDetails();
    applyRowLayout(svg,rowCapacity.value,showUnconnected.checked);
    box.scrollTop=0;box.scrollLeft=0;fit();
}
