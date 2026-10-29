"""Real-process queue tests; no Unreal processes are launched."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
import unreal_coordination as queue


def wait_for(predicate):
    until=time.monotonic()+10
    while not predicate():
        if time.monotonic()>until:raise AssertionError('Queue worker did not reach the expected state')
        time.sleep(.02)


def worker(root,name):
    root=Path(root)
    with queue.engine_slot(name,root=root/'queue',processes=lambda: ['manual editor'] if (root/'editor').exists() else [],notify=lambda _:None,poll=.02):
        with (root/'order').open('a') as stream:stream.write(name+'\n')
        (root/(name+'.started')).touch()
        if name=='nested':
            with queue.engine_slot('nested child',root=root/'queue',processes=lambda: (_ for _ in ()).throw(AssertionError('Nested call checked own engine')),notify=lambda _:None):pass
            subprocess.run([sys.executable,'-B',__file__,'inherited',str(root)],check=True,timeout=10)
        wait_for(lambda:(root/(name+'.release')).exists())


class QueueTests(unittest.TestCase):
    def setUp(self):
        owner=Path(__file__).resolve().parents[1]/'Tmp/UnrealCoordination';owner.mkdir(parents=True,exist_ok=True)
        self.temp=tempfile.TemporaryDirectory(dir=owner);self.root=Path(self.temp.name);self.children=[]
    def tearDown(self):
        for child in self.children:
            if child.poll() is None:child.kill()
            child.wait(timeout=10)
        self.temp.cleanup()
    def launch(self,name):
        env=os.environ.copy();env.pop(queue.TOKEN,None)
        child=subprocess.Popen([sys.executable,'-B',__file__,'worker',str(self.root),name],env=env)
        self.children.append(child);return child
    def tickets(self):return list((self.root/'queue').glob('*.ticket.json'))
    def release(self,name):(self.root/(name+'.release')).touch()
    def test_fifo_and_no_overlapping_work(self):
        a=self.launch('a');wait_for(lambda:(self.root/'a.started').exists())
        b=self.launch('b');wait_for(lambda:len(self.tickets())==2)
        c=self.launch('c');wait_for(lambda:len(self.tickets())==3)
        self.assertFalse((self.root/'b.started').exists());self.assertFalse((self.root/'c.started').exists())
        self.release('a');self.assertEqual(0,a.wait(timeout=10));wait_for(lambda:(self.root/'b.started').exists())
        self.assertFalse((self.root/'c.started').exists());self.release('b');self.assertEqual(0,b.wait(timeout=10))
        wait_for(lambda:(self.root/'c.started').exists());self.release('c');self.assertEqual(0,c.wait(timeout=10))
        self.assertEqual(['a','b','c'],(self.root/'order').read_text().splitlines())
        self.assertEqual([],self.tickets())
    def test_dead_owner_and_dead_waiter_release_queue(self):
        a=self.launch('a');wait_for(lambda:(self.root/'a.started').exists())
        b=self.launch('b');wait_for(lambda:len(self.tickets())==2)
        c=self.launch('c');wait_for(lambda:len(self.tickets())==3)
        b.kill();b.wait(timeout=10);a.kill();a.wait(timeout=10)
        wait_for(lambda:(self.root/'c.started').exists());self.release('c');self.assertEqual(0,c.wait(timeout=10))
        self.assertEqual(['a','c'],(self.root/'order').read_text().splitlines())
    def test_manual_editor_wait_and_nested_lease(self):
        (self.root/'editor').touch();child=self.launch('nested')
        wait_for(lambda:len(self.tickets())==1);time.sleep(.1)
        self.assertFalse((self.root/'nested.started').exists())
        (self.root/'editor').unlink();wait_for(lambda:(self.root/'nested.started').exists())
        self.release('nested');self.assertEqual(0,child.wait(timeout=10))
    def test_timeout_and_failure_cleanup(self):
        with self.assertRaises(TimeoutError):
            with queue.engine_slot('timeout',root=self.root/'queue',processes=lambda:['editor'],timeout=.05,poll=.01,notify=lambda _:None):self.fail('Busy editor bypassed')
        self.assertEqual([],self.tickets())
        with self.assertRaisesRegex(ValueError,'operation failed'):
            with queue.engine_slot('failure',root=self.root/'queue',processes=lambda:[],notify=lambda _:None):raise ValueError('operation failed')
        self.assertEqual([],self.tickets());self.assertNotIn(queue.TOKEN,os.environ)


if __name__=='__main__':
    if len(sys.argv)>1 and sys.argv[1]=='worker':worker(*sys.argv[2:])
    elif len(sys.argv)>1 and sys.argv[1]=='inherited':
        with queue.engine_slot('inherited child',root=Path(sys.argv[2])/'queue',processes=lambda: (_ for _ in ()).throw(AssertionError('Child lost reservation')),notify=lambda _:None):pass
    else:unittest.main()
