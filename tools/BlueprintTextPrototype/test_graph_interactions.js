// node test_graph_interactions.js <HTML rendered from Examples/division.json>
// Exercise the generated script with a small DOM stand-in; not a browser UI test.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const page = fs.readFileSync(process.argv[2], 'utf8');
function element(dataset = {}) {
    const classes = new Set(), attributes = {};
    return {dataset, attributes, style: {}, handlers: {}, hovered:false,
        addEventListener(event,fn){this.handlers[event]=fn},
        contains(node){return node===this}, matches(){return this.hovered},
        getBoundingClientRect(){return {left:50,right:380,top:100,bottom:220}},
        closest(){return null}, focus(){document.activeElement=this},
        classList: {toggle(name, on) {if (on) classes.add(name); else classes.delete(name)},
                    contains(name) {return classes.has(name)}},
        setAttribute(name, value) {attributes[name] = value}};
}
function decode(text){return text.replace(/&quot;/g,'"').replace(/&#x27;/g,"'").replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&')}
const sections = [...page.matchAll(/<section id="([^"]+)"[^>]*>([\s\S]*?)<\/section>/g)].map(([, id, html]) => {
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
    svg.querySelectorAll = selector => selector === '.graph-node' ? nodes : edges;
    svg.contains = node => nodes.includes(node)||edges.includes(node);
    nodes.forEach(node => {node.closest = selector => selector==='.graph-node'?node:null});
    return {id, nodes, edges, svg, querySelector: () => svg};
});
assert.equal(sections.length, 4);
const handlers = {}, view = {value: ''}, direction = {value: ''};
const ui=Object.fromEntries(['detail-version','node-details','detail-text','detail-ids','copy-status','copy-detail-ids','copy-details','close-details','edge-tooltip'].map(id=>[id,element()]));
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
    querySelectorAll: selector => selector === 'section' ? sections : sections.flatMap(s => s.nodes),
    getElementById: id => id === 'view' ? view : id === 'direction' ? direction : ui[id] || sections.find(s => s.id === id),
    addEventListener: (event, fn) => {handlers[event] = fn},
};
const context = vm.createContext({document,window:{addEventListener(){}},innerWidth:1200,innerHeight:800,
    navigator:{clipboard:{async writeText(text){if(clipboardFails)throw new Error('denied');clipboard=text}}}});
vm.runInContext(page.match(/<script>([\s\S]*?)<\/script>/)[1], context);
const background = {closest: () => null};
function cleared(section) {
    for (const item of [...section.nodes, ...section.edges]) {
        for (const name of ['muted', 'selected', 'neighbor', 'related']) assert.equal(item.classList.contains(name), false);
    }
    section.nodes.forEach(node => assert.equal(node.attributes['aria-pressed'], 'false'));
}
for (const section of sections) {
    [view.value, direction.value] = section.id.split('-');
    context.choose();
    cleared(section);
    const node = section.nodes.find(n => n.dataset.nodeId === 'sequence');
    assert.ok(node, 'Use the division example fixture');
    handlers.click({target: node});
    assert.equal(node.classList.contains('selected'), true);
    assert.equal(node.attributes['aria-pressed'], 'true');
    assert.equal(section.nodes.find(n => n.dataset.nodeId === 'divide').classList.contains('muted'), true);
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
}
console.log('Passed: incoming/outgoing links, immediate neighbors, all clear actions, keyboard, view reset, zoom/fit in all four variants (DOM stand-in).');

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
    assert.equal(JSON.parse(ui['detail-text'].textContent).id,'sequence');
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
    console.log('Passed: node panel, multi-ID/contents copy, clipboard fallback, immediate edge-click details, viewport placement and dismissal (DOM stand-in).');
})().catch(error=>{console.error(error);process.exitCode=1});
