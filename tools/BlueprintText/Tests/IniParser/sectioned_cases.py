"""Reuse the flat parser's typed-value cases inside a named namespace."""
import json
from pathlib import Path
import sys


def main():
    suite = json.loads(Path(__file__).with_name('cases.json').read_text(encoding='utf-8'))
    for step in suite['steps']:
        if step.get('method', '').startswith('Get'):
            step.setdefault('args', {})['Section'] = 'Typed.Values:#=Проверка'
        if step.get('write'):
            # The flat suite deliberately includes an ignored header. Its values
            # all belong to this one namespace in the sectioned numeric regression.
            text = step['write'].lstrip('\ufeff').replace('[Section]\r\n', '')
            step['write'] = '\ufeff[Typed.Values:#=Проверка]\r\n' + text
    Path(sys.argv[1]).write_text(json.dumps(suite, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
