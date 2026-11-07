// node test_graph_interactions.js <HTML rendered from Examples/division.json>
// Exercise the generated script with a small DOM stand-in; not a browser UI test.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const page = fs.readFileSync(process.argv[2], 'utf8');
let forbidGraphScan=false;
let mutations=0;
function element(dataset = {}) {
    const classes = new Set(), attributes = {};
    return {dataset, attributes, style: {}, handlers: {}, hovered:false,
        addEventListener(event,fn){this.handlers[event]=fn},
        contains(node){return node===this}, matches(){return this.hovered},
        getBoundingClientRect(){return {left:50,right:380,top:100,bottom:220}},
        closest(){return null}, focus(){document.activeElement=this},
        classList: {toggle(name, on) {mutations++;if (on) classes.add(name); else classes.delete(name)},
                    contains(name) {return classes.has(name)}},
        setAttribute(name, value) {mutations++;attributes[name] = value}};
}
function decode(text){return text.replace(/&quot;/g,'"').replace(/&#x27;/g,"'").replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&')}
const allSections = [...page.matchAll(/<section id="([^"]+)"[^>]*>([\s\S]*?)<\/section>/g)].map(([, id, html]) => {
    const nodes = [...html.matchAll(/class="graph-node" data-node-id="([^"]+)" data-node-ids="([^"]+)"[^>]*><title>([\s\S]*?)<\/title>/g)].map(([, nodeId,nodeIds,text]) => {
        const node=element({nodeId:decode(nodeId),nodeIds:decode(nodeIds)});
        const title={textContent:decode(text),remove(){this.removed=true}};
        node.querySelector=()=>title;
        node.icon=element();node.icon.closest=selector=>selector==='.copy-node-id'?node.icon:selector==='.graph-node'?node:null;
        node.info=element();node.info.closest=selector=>selector==='.node-info'?node.info:selector==='.graph-node'?node:null;
        node.contains=target=>target===node||target===node.icon||target===node.info;
        return node;
    });
    const edges = [...html.matchAll(/class="graph-edge" data-source="([^"]+)" data-target="([^"]+)"><title>([\s\S]*?)<\/title>/g)].map(([, source, target, text]) => {
        const edge=element({source,target}),title={textContent:decode(text)};edge.querySelector=()=>title;
        edge.closest=selector=>selector==='.graph-edge'?edge:null;
        edge.hit=element();edge.hit.closest=edge.closest;
        return edge;
    });
    const svg = element();
    svg.viewBox = {baseVal: {width: 1400, height: 2200}};
    svg.querySelectorAll = selector => {
        assert.equal(forbidGraphScan,false,'Highlight must use the cached index, not scan SVG elements');
        return selector === '.graph-node' ? nodes : edges;
    };
    svg.contains = node => nodes.includes(node)||edges.includes(node);
    nodes.forEach(node => {node.closest = selector => selector==='.graph-node'?node:null});
    return {id, nodes, edges, svg, querySelector: () => svg};
});
const sections=allSections.filter(s=>!s.id.startsWith('final-'));
assert.equal(sections.length, 6);
const handlers = {}, view = {value: ''}, direction = {value: ''};
const ui=Object.fromEntries(['unconnected','nodes-per-row','detail-version','node-details','detail-text','detail-ids','copy-status','copy-detail-ids','copy-details','close-details','edge-tooltip'].map(id=>[id,element()]));
ui['unconnected'].checked=true;
if(page.includes('id="show-changes"')){ui['show-changes']=element();ui['show-changes'].checked=true}
if(page.includes('id="caller-menu"')){
    const menu=ui['caller-menu']=element(),link=element();
    menu.querySelector=()=>link;menu.link=link;menu.hidden=true;menu.offsetWidth=300;
    menu.contains=target=>[menu,link].includes(target);
    link.closest=selector=>selector==='a'?link:null;
    link.getAttribute=()=> 'caller.html';
}
const edgeTip=ui['edge-tooltip'];edgeTip.hidden=true;edgeTip.offsetWidth=400;edgeTip.offsetHeight=120;
const tip=ui['node-details'];tip.hidden=true;tip.offsetWidth=520;tip.offsetHeight=400;
tip.contains=node=>Object.values(ui).includes(node);
const box = {clientWidth: 900, clientHeight: 700,
             getBoundingClientRect(){return {left:10,top:60}},
             addEventListener: (event, fn) => {handlers[event] = fn}};
let clipboard='',clipboardFails=false,fallbackFails=false,textarea;
const document = {
    activeElement:null,
    body:{appendChild(area){textarea=area}},
    createElement(){return Object.assign(element(),{select(){},remove(){}})},
    execCommand(){if(fallbackFails)return false;clipboard=textarea.value;return true},
    querySelector: () => box,
    querySelectorAll: selector => selector === 'section' ? allSections : allSections.flatMap(s => s.nodes),
    getElementById: id => id === 'view' ? view : id === 'direction' ? direction : ui[id] || allSections.find(s => s.id === id),
    addEventListener: (event, fn, capture) => {handlers[(capture?'capture-':'')+event] = fn},
};
const windowHandlers={},history={state:{unrelated:'keep'},replaceState(state){this.state=structuredClone(state)}};
const location={href:'file:///preview/parent.html'};
const context = vm.createContext({document,window:{addEventListener(event,fn){windowHandlers[event]=fn}},innerWidth:1200,innerHeight:800,
    history,location,URL,setTimeout,clearTimeout,
    navigator:{clipboard:{async writeText(text){if(clipboardFails)throw new Error('denied');clipboard=text}}}});
vm.runInContext(page.match(/<script>([\s\S]*?)<\/script>/)[1], context);
const background = {closest: () => null};
if(ui['caller-menu']){
    const menu=ui['caller-menu'],button=element();
    button.closest=selector=>selector==='.open-caller-graph'?button:selector==='.graph-node'?sections[0].nodes[0]:null;
    handlers.click({target:button,stopPropagation(){}});
    assert.equal(menu.hidden,false);assert.equal(button.attributes['aria-expanded'],'true');
    assert.equal(document.activeElement,menu.link);
    handlers['capture-click']({target:background});assert.equal(menu.hidden,true);
    handlers.keydown({key:'Enter',target:button,preventDefault(){}});
    assert.equal(menu.hidden,false);let prevented=false;
    handlers.keydown({key:'Escape',target:menu.link,preventDefault(){prevented=true}});
    assert.equal(menu.hidden,true);assert.equal(document.activeElement,button);assert.ok(prevented);
    assert.equal(button.attributes['aria-expanded'],'false');
    handlers.click({target:button,stopPropagation(){}});
    handlers['capture-click']({target:menu.link});assert.equal(menu.hidden,true);
}
{
    const node=sections[0].nodes[0],member=JSON.parse(node.dataset.nodeIds).at(-1);
    history.state=null;location.href='file:///preview/caller.html#node-'+encodeURIComponent(member);
    context.restoreViewerState();
    assert.equal(node.classList.contains('selected'),true,'Navigate to the containing chain for a folded call');
    box.scrollLeft=123;box.scrollTop=456;context.saveViewerState();
    windowHandlers.hashchange();
    assert.equal(box.scrollLeft,123);assert.equal(box.scrollTop,456,'Hash navigation must not overwrite restored history scroll');
    history.state={unrelated:'keep'};location.href='file:///preview/parent.html';context.restoreViewerState();
}
{
    const initialView=view.value,initialDirection=direction.value;
    const full=sections.find(s=>s.id==='full-TB'),target=full.nodes.at(-1),originalIds=target.dataset.nodeIds;
    // Simulate a source node represented only in Full, not in the simplified view.
    target.dataset.nodeIds=JSON.stringify([...JSON.parse(originalIds),'hidden search target']);
    view.value='simplified';direction.value='TB';context.choose();history.state=null;
    location.href='file:///preview/graph.html#node-hidden%20search%20target';context.focusLinkedNode();
    assert.equal(view.value,'full');assert.equal(target.classList.contains('selected'),true);
    target.dataset.nodeIds=originalIds;
    view.value='simplified';context.choose();
    location.href='file:///preview/graph.html#node-missing';context.focusLinkedNode();
    assert.equal(view.value,'simplified','Unknown IDs must not change view');
    const folded=sections.find(s=>s.id==='simplified-TB').nodes[0],foldedIds=folded.dataset.nodeIds;
    folded.dataset.nodeIds=JSON.stringify([...JSON.parse(foldedIds),'folded search target']);
    location.href='file:///preview/graph.html#node-folded%20search%20target';context.focusLinkedNode();
    assert.equal(folded.classList.contains('selected'),true);assert.equal(view.value,'simplified');
    folded.dataset.nodeIds=foldedIds;
    view.value=initialView;direction.value=initialDirection;
    history.state={unrelated:'keep'};location.href='file:///preview/parent.html';context.restoreViewerState();
}
function cleared(section) {
    assert.equal(section.svg.classList.contains('has-selection'),false);
    for (const item of [...section.nodes, ...section.edges]) {
        for (const name of ['muted', 'selected', 'neighbor', 'related']) assert.equal(item.classList.contains(name), false);
    }
    section.nodes.forEach(node => assert.equal(node.attributes['aria-pressed'], 'false'));
}
for (const section of sections) {
    [view.value, direction.value] = section.id.split('-');
    context.choose();
    const navigationLink=element();
    navigationLink.closest=selector=>selector==='a'?navigationLink:selector==='.graph-node'?section.nodes[0]:null;
    handlers.click({target:navigationLink});
    handlers.keydown({target:navigationLink,key:'Enter',preventDefault(){assert.fail('Graph navigation must retain native keyboard activation')}});
    cleared(section);
    forbidGraphScan=true;
    ui['unconnected'].checked=false;context.changeUnconnected();
    assert.equal(section.svg.classList.contains('hide-unconnected'),true);
    context.choose();assert.equal(section.svg.classList.contains('hide-unconnected'),true);
    ui['unconnected'].checked=true;context.changeUnconnected();
    assert.equal(section.svg.classList.contains('hide-unconnected'),false);
    assert.equal(ui['nodes-per-row'].disabled,direction.value!=='TB');
    cleared(section);
    const node = section.nodes.find(n => n.dataset.nodeId === 'sequence');
    assert.ok(node, 'Use the division example fixture');
    handlers.click({target: node});
    assert.equal(node.classList.contains('selected'), true);
    assert.equal(node.attributes['aria-pressed'], 'true');
    const selectedWidth=section.svg.style.width;
    box.scrollLeft=800;box.scrollTop=1000;
    ui['unconnected'].checked=false;context.changeUnconnected();
    assert.equal(box.scrollLeft,555,'Center selected node horizontally after reflow');
    assert.equal(box.scrollTop,750,'Center selected node vertically after reflow');
    assert.equal(section.svg.style.width,selectedWidth,'Retain zoom for selected node');
    assert.equal(node.classList.contains('selected'),true);
    ui['unconnected'].checked=true;context.changeUnconnected();
    assert.equal(section.svg.classList.contains('has-selection'),true);
    const unrelated=section.nodes.find(n => n.dataset.nodeId === 'divide');
    assert.equal(unrelated.classList.contains('selected')||unrelated.classList.contains('neighbor'),false);
    assert.match(page,/svg\.has-selection \.graph-node:not\(\.selected\):not\(\.neighbor\)/);
    for (const id of ['begin', 'set.result']) assert.equal(section.nodes.find(n => n.dataset.nodeId === id).classList.contains('neighbor'), true);
    assert.deepEqual(section.edges.filter(e => e.classList.contains('related')).map(e => [e.dataset.source, e.dataset.target]),
                     [['begin', 'sequence'], ['sequence', 'set.result']]);
    // Both incoming links of Set are selected; the neighbor's other edge is not.
    const set = section.nodes.find(n => n.dataset.nodeId === 'set.result');
    handlers.click({target: set});
    assert.deepEqual(section.edges.filter(e => e.classList.contains('related')).map(e => e.dataset.source), ['divide', 'sequence']);
    handlers.click({target: set}); cleared(section);
    handlers.click({target: node}); handlers.click({target: background}); cleared(section);
    handlers.click({target: node}); handlers.keydown({key: 'Escape', target: node}); cleared(section);
    for (const key of ['Enter', ' ']) {
        let prevented = false;
        handlers.keydown({key, target: node, preventDefault() {prevented = true}});
        assert.equal(prevented, true); assert.equal(node.classList.contains('selected'), true);
        handlers.keydown({key, target: node, preventDefault() {}}); cleared(section);
    }
    handlers.click({target: node}); context.choose(); cleared(section);
    const width = parseFloat(section.svg.style.width);
    context.zoom(1.25); assert.ok(parseFloat(section.svg.style.width) > width);
    context.fit(true); assert.ok(parseFloat(section.svg.style.height) <= box.clientHeight);
    forbidGraphScan=false;
}
console.log('Passed: incoming/outgoing links, immediate neighbors, all clear actions, keyboard, view reset, zoom/fit in all six variants (DOM stand-in).');
// A large sparse graph must touch only the selected neighborhood after indexing.
{
    const nodes=Array.from({length:5000},(_,i)=>element({nodeId:String(i)}));
    const edges=nodes.slice(1,-1).map((_,i)=>element({source:String(i),target:String(i+1)}));
    edges.push(element({source:'20',target:'20'}),element({source:'20',target:'21'}));
    const graph=element();let scans=0;
    graph.querySelectorAll=selector=>{assert.equal(forbidGraphScan,false);scans++;return selector==='.graph-node'?nodes:edges};
    const previous=vm.runInContext('svg',context);context.largeGraph=graph;
    vm.runInContext('svg=largeGraph;highlight(null)',context);
    assert.equal(scans,2);forbidGraphScan=true;mutations=0;
    context.highlight('20');context.highlight('30');
    assert.ok(mutations<40,`Local selections made ${mutations} DOM mutations`);
    assert.equal(nodes[20].classList.contains('selected'),false);
    assert.equal(nodes[21].classList.contains('neighbor'),false);
    context.highlight('20');
    assert.equal(nodes[20].classList.contains('neighbor'),false,'Self-loop must not mark selected node as neighbor');
    assert.equal(edges.filter(e=>e.classList.contains('related')).length,4,'Both parallel links and self-loop retained');
    context.highlight('4999');
    assert.equal(edges.some(e=>e.classList.contains('related')),false,'Isolated node has no related edges');
    context.highlight(null);assert.equal(graph.classList.contains('has-selection'),false);
    forbidGraphScan=false;context.previousGraph=previous;vm.runInContext('svg=previousGraph',context);
    console.log('Passed: 5000-node indexed selection, bounded DOM updates, parallel links, self-loop and isolated node.');
}
for(const [value,expected] of [['6','6'],['0','1'],['30','12'],['','3']]){
    ui['nodes-per-row'].value=value;context.changeColumns();
    assert.equal(ui['nodes-per-row'].value,expected);
}

(async()=>{
    view.value='simplified';direction.value='TB';context.choose();
    const node=sections[0].nodes.find(n=>n.dataset.nodeId==='sequence');
    assert.equal(handlers.mouseover,undefined,'Hover must not open details');
    assert.equal(handlers.focusin,undefined,'Tabbing onto a node must not open details');
    assert.equal(tip.hidden,true);
    box.scrollLeft=75;box.scrollTop=125;
    handlers.click({target:node.info,stopPropagation(){}});assert.equal(tip.hidden,false);
    assert.equal(tip.style.left,'115px');assert.equal(tip.style.top,'291px');
    assert.equal(node.classList.contains('selected'),false);
    const shown=JSON.parse(ui['detail-text'].textContent);
    assert.equal((shown.node||shown).id,'sequence');
    handlers.click({target:ui['detail-text']});assert.equal(tip.hidden,false,'Selecting contents does not dismiss');
    handlers.scroll();assert.equal(tip.hidden,false,'Node panel stays anchored in scrolling graph content');
    handlers.click({target:node.info,stopPropagation(){}});assert.equal(tip.hidden,true);
    handlers.keydown({target:node.info,key:'Enter',preventDefault(){}});assert.equal(tip.hidden,false);
    handlers.keydown({target:node.info,key:' ',preventDefault(){}});assert.equal(tip.hidden,true);
    let stopped=false;
    handlers.click({target:node.icon,stopPropagation(){stopped=true}});
    await Promise.resolve();await Promise.resolve();
    assert.equal(stopped,true);assert.equal(clipboard,'sequence');assert.equal(node.classList.contains('selected'),false);
    // Composites copy every source ID in order, not only the representative ID.
    const original=node.dataset.nodeIds;node.dataset.nodeIds=JSON.stringify(['source','conversion']);
    await context.copyNodeIds(node);assert.equal(clipboard,'source\nconversion');
    assert.equal(ui['copy-status'].textContent,'2 node IDs copied');
    node.dataset.nodeIds=original;
    clipboardFails=true;await context.copyNodeIds(node);assert.equal(clipboard,'sequence');
    fallbackFails=true;context.closeDetails();await context.copyNodeIds(node);
    assert.equal(tip.hidden,true);assert.match(ui['copy-status'].textContent,/Open Details/);
    clipboardFails=false;fallbackFails=false;
    context.showDetails(node);ui['copy-details'].handlers.click();
    await Promise.resolve();assert.equal(clipboard,ui['detail-text'].textContent);
    // Review uses annotated After, raw Before, and clean JSON for copying.
    context.reviewTestNode=node;
    context.reviewTestData={Status:'changed',Before:{node:{value:1}},After:{node:{value:2}}};
    vm.runInContext('details.set(reviewTestNode,JSON.stringify(reviewTestData))',context);
    node.dataset.reviewDetails='<span class="detail-changed">value: 2</span>';
    context.showDetails(node);
    assert.equal(ui['detail-version'].hidden,false);
    assert.equal(ui['detail-version'].value,'After');
    assert.equal(ui['detail-text'].innerHTML,node.dataset.reviewDetails);
    ui['copy-details'].handlers.click();await Promise.resolve();
    assert.deepEqual(JSON.parse(clipboard),context.reviewTestData.After);
    ui['detail-version'].value='Before';ui['detail-version'].handlers.change();
    assert.deepEqual(JSON.parse(ui['detail-text'].textContent),context.reviewTestData.Before);
    ui['copy-details'].handlers.click();await Promise.resolve();
    assert.deepEqual(JSON.parse(clipboard),context.reviewTestData.Before);
    handlers.keydown({key:'Escape',target:node});assert.equal(tip.hidden,true);
    context.showDetails(node);context.choose();assert.equal(tip.hidden,true);
    const edge=sections[0].edges.find(e=>e.dataset.source==='divide');
    const edgeText=edge.querySelector('title').textContent;
    handlers.click({target:edge.hit,clientX:1100,clientY:750,stopPropagation(){}});
    assert.equal(edgeTip.hidden,false,'Edge details are synchronous: no hover timer');
    assert.equal(edgeTip.textContent,edgeText);
    assert.equal(edge.querySelector('title').textContent,'','No duplicate delayed tooltip while open');
    assert.equal(edgeTip.style.left,'792px');assert.equal(edgeTip.style.top,'672px');
    assert.equal(edge.classList.contains('inspected'),true);
    handlers.click({target:edge.hit,clientX:1100,clientY:750,stopPropagation(){}});
    assert.equal(edgeTip.hidden,true);assert.equal(edge.classList.contains('inspected'),false);
    assert.equal(edge.querySelector('title').textContent,edgeText,'Restore native hover after closing');
    for(const dismiss of [()=>handlers.scroll(),()=>handlers.keydown({key:'Escape',target:background}),
                          ()=>handlers.click({target:background}),()=>context.choose(),()=>context.showDetails(node)]){
        handlers.click({target:edge.hit,clientX:200,clientY:300,stopPropagation(){}});
        assert.equal(edgeTip.hidden,false);dismiss();assert.equal(edgeTip.hidden,true);
    }
    assert.ok(node.querySelector('title').removed,'Replace native unselectable tooltip');
    assert.match(page, /#node-details\{position:absolute;/);
    assert.match(page, /<aside id="node-details"[\s\S]*<\/aside><\/main>/);
    for(const mode of ['full','expressions']){
    history.state={...history.state,unrelated:'keep'};
    if(ui['show-changes'])ui['show-changes'].checked=true;
    view.value=mode;direction.value='LR';ui['nodes-per-row'].value='5';ui.unconnected.checked=false;
    context.choose();context.zoom(1.25);context.highlight('sequence');
    box.scrollLeft=123;box.scrollTop=456;
    windowHandlers.pagehide();
    const saved=structuredClone(history.state),savedWidth=sections.find(s=>s.id===mode+'-LR').svg.style.width;
    assert.equal(saved.unrelated,'keep');
    history.state=null;view.value='simplified';direction.value='TB';ui.unconnected.checked=true;context.choose();
    history.state=saved;windowHandlers.pageshow();
    assert.equal(view.value,mode);assert.equal(direction.value,'LR');assert.equal(ui.unconnected.checked,false);
    assert.equal(ui['nodes-per-row'].value,'5');assert.equal(box.scrollLeft,123);assert.equal(box.scrollTop,456);
    assert.equal(sections.find(s=>s.id===mode+'-LR').svg.style.width,savedWidth);
    assert.equal(sections.find(s=>s.id===mode+'-LR').nodes.find(n=>n.dataset.nodeId==='sequence').attributes['aria-pressed'],'true');
    box.scrollLeft=0;windowHandlers.popstate();assert.equal(box.scrollLeft,123);
    if(ui['show-changes']){
        ui['show-changes'].checked=false;context.changeReview();
        assert.equal(allSections.find(s=>s.id==='final-'+mode+'-LR').hidden,false);
        assert.equal(allSections.find(s=>s.id===mode+'-LR').hidden,true);
        windowHandlers.pagehide();const cleanState=structuredClone(history.state);
        ui['show-changes'].checked=true;context.changeReview();history.state=cleanState;windowHandlers.pageshow();
        assert.equal(ui['show-changes'].checked,false);assert.equal(box.scrollTop,456);
        const link={href:{baseVal:'file:///preview/child.html'},closest(){return this},
            getAttribute(){return this.href.baseVal},setAttribute(k,v){this.href.baseVal=v}};
        handlers['capture-click']({target:link});assert.match(link.getAttribute('href'),/\?r=0$/);
        ui['show-changes'].checked=true;context.changeReview();handlers['capture-click']({target:link});assert.ok(!link.getAttribute('href').includes('?'));
        history.state=null;location.href='file:///preview/child.html?r=0';context.restoreViewerState();
        assert.equal(ui['show-changes'].checked,false,'Clean mode follows navigation into a child');
    }
    }
    context.saveViewerState();
    console.log('Passed: graph interactions, per-entry history restoration, scroll/zoom/filters and final-only switching (DOM stand-in).');
})().catch(error=>{console.error(error);process.exitCode=1});
