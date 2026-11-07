// Root-page search over represented BPT nodes; no fetch or asset discovery.
(()=>{
    const records=JSON.parse(document.getElementById('node-search-data').textContent);
    const query=document.getElementById('node-search-query');
    const before=document.getElementById('node-search-before');
    const results=document.getElementById('node-search-results');
    const status=document.getElementById('node-search-status');
    const hasBefore=records.some(n=>n.snapshot==='Before');
    document.getElementById('node-search-before-label').hidden=!hasBefore;
    const indexed=records.map(n=>({node:n,text:[n.id,n.title,n.path,n.asset].join(' ').toLowerCase()}));
    let matches=[];
    function render(){
        const value=query.value.trim().toLowerCase(),terms=value.split(/\s+/).filter(Boolean);
        results.replaceChildren();
        matches=terms.length?indexed.filter(r=>(r.node.snapshot!=='Before'||before.checked)&&terms.every(t=>r.text.includes(t))).map(r=>r.node):[];
        const score=n=>n.id.toLowerCase()===value?0:n.id.toLowerCase().startsWith(value)?1:2;
        matches.sort((a,b)=>score(a)-score(b)||(a.snapshot==='Before')-(b.snapshot==='Before')||a.path.localeCompare(b.path)||a.id.localeCompare(b.id));
        status.textContent=!terms.length?'Enter a node ID or name.':!matches.length?'No matching nodes.':
            `${matches.length} match${matches.length===1?'':'es'}. ${matches.length>100?'Showing the first 100; refine the search.':'Choose the Blueprint and graph path.'}`;
        for(const node of matches.slice(0,100)){
            const li=document.createElement('li'),link=document.createElement('a');
            link.href=node.href;link.title=node.asset;
            const id=document.createElement('code');id.textContent=node.id;
            const name=document.createElement('span');name.className='node-name';name.textContent=node.title;
            const path=document.createElement('small');path.textContent=`${node.path} · ${node.snapshot}`;
            link.append(id,name,path);li.append(link);results.append(li);
        }
    }
    function save(){
        const url=new URL(location.href);
        if(query.value.trim())url.searchParams.set('q',query.value.trim());else url.searchParams.delete('q');
        if(before.checked)url.searchParams.set('before','1');else url.searchParams.delete('before');
        try{history.replaceState(history.state,'',url.href)}catch(error){/* The search also works in restricted embeds. */}
    }
    function restore(){
        const params=new URL(location.href).searchParams;
        query.value=params.get('q')||'';before.checked=hasBefore&&params.get('before')==='1';render();
    }
    query.addEventListener('input',()=>{render();save()});
    before.addEventListener('change',()=>{render();save()});
    function clear(){query.value='';render();save();query.focus()}
    document.getElementById('node-search-clear').addEventListener('click',clear);
    query.addEventListener('keydown',event=>{
        if(event.key==='Escape'){event.preventDefault();clear()}
        if(event.key==='Enter'&&matches.length){
            event.preventDefault();
            if(matches.length===1)location.assign(matches[0].href);
            else results.querySelector('a')?.focus();
        }
    });
    window.addEventListener('pageshow',restore);
    window.addEventListener('popstate',restore);
    restore();
})();
