"""FIFO lease for Unreal work across projects/repository copies on this user account."""
import argparse
from contextlib import contextmanager
import csv
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import uuid

ROOT = Path(tempfile.gettempdir()) / 'Codex-Unreal-Queue'
TOKEN = 'CODEX_UNREAL_QUEUE_LEASE'


def try_lock(path):
    stream = Path(path).open('a+b')
    if stream.seek(0, 2) == 0:
        stream.write(b' '); stream.flush()
    stream.seek(0)
    try:
        if os.name == 'nt':
            import msvcrt
            msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return stream
    except (BlockingIOError, PermissionError):
        stream.close()
        return None


@contextmanager
def mutex(path):
    stream = None
    try:
        while stream is None:
            stream = try_lock(path)
            if stream is None: time.sleep(.1)
        yield
    finally:
        if stream: stream.close()


def running_editors():
    if os.name != 'nt':
        raise RuntimeError('Unreal process detection is supported on Windows only')
    result = subprocess.run(['tasklist.exe','/FO','CSV','/NH'], capture_output=True, text=True, check=True)
    return [dict(name=row[0],pid=int(row[1])) for row in csv.reader(result.stdout.splitlines())
            if len(row)>1 and row[0].lower() in ('unrealeditor.exe','unrealeditor-cmd.exe')]


def live_tickets(root):
    result = []
    # Caller holds the scheduling mutex; no live owner can be mistaken for stale.
    for path in sorted(root.glob('*.ticket.json')):
        lease = path.with_suffix('.lease')
        probe = try_lock(lease)
        if probe is not None:
            probe.close(); path.unlink(); lease.unlink(missing_ok=True)
        else:
            result.append((path,json.loads(path.read_text(encoding='utf-8'))))
    return result


def emit(event):
    print(json.dumps(dict(unrealQueue=event)),file=sys.stderr,flush=True)


@contextmanager
def engine_slot(operation, *, root=None, processes=None, notify=emit, timeout=None, poll=1):
    """Wait, do not fail on contention. Nested synchronous calls inherit the lease."""
    root = Path(root or ROOT).resolve(); root.mkdir(parents=True,exist_ok=True)
    processes = processes or running_editors
    scheduling = root/'scheduler.lock'
    inherited = os.environ.get(TOKEN)
    if inherited:
        with mutex(scheduling):
            tickets=live_tickets(root)
            if any(record['token']==inherited and record.get('state')=='running' for _,record in tickets):
                nested=True
            else: nested=False
        if nested:
            yield
            return
        raise RuntimeError('Inherited Unreal lease has expired; restart through the queue')
    ticket = None; lease = None; previous = None; start=time.monotonic()
    try:
        with mutex(scheduling):
            counter=root/'sequence.txt'
            sequence=int(counter.read_text())+1 if counter.exists() else 1
            temporary=counter.with_suffix('.writing')
            temporary.write_text(str(sequence));temporary.replace(counter)
            token=uuid.uuid4().hex
            ticket=root/f'{sequence:020d}-{token}.ticket.json'
            lease=try_lock(ticket.with_suffix('.lease'))
            if lease is None: raise RuntimeError('Cannot hold new queue ticket')
            record=dict(token=token,pid=os.getpid(),operation=operation,state='waiting')
            ticket.write_text(json.dumps(record),encoding='utf-8')
        while True:
            acquired=False
            with mutex(scheduling):
                tickets=live_tickets(root)
                position=next(i for i,(path,_) in enumerate(tickets) if path==ticket)
                if position==0:
                    editors=processes()
                    if not editors:
                        record['state']='running';ticket.write_text(json.dumps(record),encoding='utf-8');acquired=True
                    state=dict(status='waiting-editor',processes=editors) if editors else dict(status='acquired',operation=operation)
                else:
                    head=tickets[0][1]
                    state=dict(status='queued',ahead=position,owner=head['operation'],pid=head['pid'])
            if state!=previous: notify(state);previous=state
            if acquired:break
            if timeout is not None and time.monotonic()-start>=timeout:
                raise TimeoutError('Unreal queue wait timed out; no engine work started')
            time.sleep(poll)
        os.environ[TOKEN]=record['token']
        try: yield
        finally: os.environ.pop(TOKEN,None)
    finally:
        if ticket:
            with mutex(scheduling):
                ticket.unlink(missing_ok=True)
                if lease:lease.close()
                ticket.with_suffix('.lease').unlink(missing_ok=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--operation',required=True)
    parser.add_argument('--timeout',type=float)
    parser.add_argument('command',nargs=argparse.REMAINDER)
    args=parser.parse_args();command=args.command
    if command and command[0]=='--':command=command[1:]
    if not command:parser.error('Provide an executable and arguments after --')
    with engine_slot(args.operation,timeout=args.timeout):
        return subprocess.call(command)


if __name__=='__main__':sys.exit(main())
