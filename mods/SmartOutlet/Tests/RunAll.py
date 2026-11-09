"""Run compiled Blueprint checks in one editor session; no native game simulation."""
import json
from pathlib import Path
import runpy

tests = Path(__file__).resolve().parent
output = tests.parents[2] / 'Tmp/SmartOutlet'
output.mkdir(parents=True, exist_ok=True)
summary = output / 'checks-validation.json'
summary.unlink(missing_ok=True)
cases = [
    ('ValidateEditor', 'editor-validation.json'),
    ('ValidateRules', 'rules-validation.json'),
    ('ValidateResearchTier', 'research-tier-validation.json'),
    ('ValidateLatchState', 'latch-validation.json'),
    ('ValidatePortInteraction', 'port-interaction-validation.json'),
    ('ValidateModes', 'modes-validation.json'),
]
for _, report in cases:
    (output / report).unlink(missing_ok=True)
results = []
for script, report in cases:
    runpy.run_path(str(tests / (script + '.py')), run_name='__main__')
    result = json.loads((output / report).read_text(encoding='utf-8'))
    assert result['status'] == 'passed', script
    results.append({'test': script, 'status': result['status'], 'report': str(output / report)})
summary.write_text(json.dumps({'status': 'passed', 'tests': results,
    'boundary': 'Compiled Blueprints against editor mirrors; not native game transfer or saves'},
    indent=2) + '\n', encoding='utf-8')
