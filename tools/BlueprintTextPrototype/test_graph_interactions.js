// node test_graph_interactions.js <HTML rendered from Examples/division.json>
// Exercise the generated script with a small DOM stand-in; not a browser UI test.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const page = fs.readFileSync(process.argv[2], 'utf8');
function element(dataset = {}) {
    const classes = new Set(), attributes = {};
    return {dataset, attributes, style: {},
        classList: {toggle(name, on) {if (on) classes.add(name); else classes.delete(name)},
                    contains(name) {return classes.has(name)}},
        setAttribute(name, value) {attributes[name] = value}};
}
const sections = [...page.matchAll(/<section id="([^"]+)"[^>]*>([\s\S]*?)<\/section>/g)].map(([, id, html]) => {
    const nodes = [...html.matchAll(/class="graph-node" data-node-id="([^"]+)"/g)].map(([, nodeId]) => element({nodeId}));
    const edges = [...html.matchAll(/class="graph-edge" data-source="([^"]+)" data-target="([^"]+)"/g)].map(([, source, target]) => element({source, target}));
    const svg = element();
    svg.viewBox = {baseVal: {width: 1400, height: 2200}};
    svg.querySelectorAll = selector => selector === '.graph-node' ? nodes : edges;
    svg.contains = node => nodes.includes(node);
    nodes.forEach(node => {node.closest = () => node});
    return {id, nodes, edges, svg, querySelector: () => svg};
});
assert.equal(sections.length, 4);
const handlers = {}, view = {value: ''}, direction = {value: ''};
const box = {clientWidth: 900, clientHeight: 700,
             addEventListener: (event, fn) => {handlers[event] = fn}};
const document = {
    querySelector: () => box,
    querySelectorAll: selector => selector === 'section' ? sections : sections.flatMap(s => s.nodes),
    getElementById: id => id === 'view' ? view : id === 'direction' ? direction : sections.find(s => s.id === id),
    addEventListener: (event, fn) => {handlers[event] = fn},
};
const context = vm.createContext({document});
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
