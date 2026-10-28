// Embedded in generated HTML; no external scripts or network dependencies.
const box=document.querySelector('main'),view=document.getElementById('view'),direction=document.getElementById('direction');
const tip=document.getElementById('node-details'),detailText=document.getElementById('detail-text');
const detailIds=document.getElementById('detail-ids'),status=document.getElementById('copy-status');
const edgeTip=document.getElementById('edge-tooltip');
let svg,scale=1,selectedId=null,detailNode=null,detailEdge=null,detailEdgeTitle=null;
const details=new WeakMap();
function closeEdgeDetails(){
    if(detailEdgeTitle)detailEdgeTitle.textContent=edgeTip.textContent;
    edgeTip.hidden=true;detailEdge?.classList.toggle('inspected',false);detailEdge=null;detailEdgeTitle=null;
}
function closeDetails(){tip.hidden=true;detailNode=null;closeEdgeDetails()}
function showDetails(node){
    closeEdgeDetails();
    detailNode=node;detailText.textContent=details.get(node);
    detailIds.textContent=JSON.parse(node.dataset.nodeIds).join(' → ');
    tip.hidden=false;
    const r=node.getBoundingClientRect(),container=box.getBoundingClientRect();
    // The panel scrolls with the graph and never flips above/over the source node.
    tip.style.left=(r.left-container.left+box.scrollLeft)+'px';
    tip.style.top=(r.bottom-container.top+box.scrollTop+6)+'px';
}
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
function highlight(id){
    selectedId=id;
    const neighbors=new Set(id===null?[]:[id]);
    svg.querySelectorAll('.graph-edge').forEach(edge=>{
        const related=id!==null&&(edge.dataset.source===id||edge.dataset.target===id);
        edge.classList.toggle('related',related);edge.classList.toggle('muted',id!==null&&!related);
        if(related){neighbors.add(edge.dataset.source);neighbors.add(edge.dataset.target)}
    });
    svg.querySelectorAll('.graph-node').forEach(node=>{
        const selected=node.dataset.nodeId===id,neighbor=!selected&&neighbors.has(node.dataset.nodeId);
        node.classList.toggle('selected',selected);node.classList.toggle('neighbor',neighbor);
        node.classList.toggle('muted',id!==null&&!neighbors.has(node.dataset.nodeId));
        node.setAttribute('aria-pressed',String(selected));
    });
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
document.getElementById('copy-details').addEventListener('click',()=>copyText(detailText.textContent,'Node details copied'));
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
    node.setAttribute('aria-label','Highlight connections for '+JSON.parse(node.dataset.nodeIds).join(', '));
});
function choose(){
    closeDetails();const id=view.value+'-'+direction.value;
    document.querySelectorAll('section').forEach(s=>s.hidden=s.id!==id);
    svg=document.getElementById(id).querySelector('svg');highlight(null);box.scrollTop=0;box.scrollLeft=0;fit();
}
