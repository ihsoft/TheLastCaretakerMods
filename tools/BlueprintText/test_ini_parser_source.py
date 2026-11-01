"""Public contract and runtime dependency checks for the reusable BPT source."""
import copy
import json
from pathlib import Path
import re
import unittest

import asset_text_codec as codec


class IniParserSourceTests(unittest.TestCase):
    def setUp(self):
        self.source = json.loads((Path(__file__).parent / 'Library/IniFileParser/BP_IniFileParser.json').read_text(encoding='utf-8'))
        self.graphs = {g['name']: g for g in self.source['graphs']}

    def test_plain_object_state_and_relocation(self):
        self.assertEqual(self.source['parent'], '/Script/CoreUObject.Object')
        self.assertFalse(set(self.source) & {'root', 'components', 'widget'})
        self.assertEqual({codec.unquote(v['VarName']) for v in self.source['variables']}, {'FilePath', 'Values'})
        before = copy.deepcopy(self.source)
        for mod in ('One', 'Two'):
            moved = codec.relocate(self.source, f'/Game/Mods/{mod}/BP_IniFileParser')
            self.assertFalse(codec.compare(moved, codec.compact(codec.native(moved))))
        self.assertEqual(self.source, before)

    def test_getters_are_pure_and_cannot_reach_disk_or_cache_mutation(self):
        def visit(name, seen):
            if name in seen:
                return
            seen.add(name)
            for node in self.graphs[name]['body']['nodes']:
                match = re.search(r'MemberName="([^"]+)"', node['properties'].get('FunctionReference', ''))
                if match:
                    function = match[1]
                    self.assertNotIn(function, ('LoadFileToArray', 'Map_Add', 'Map_Remove', 'Map_Clear', 'Load', 'Reload'))
                    if function in self.graphs:
                        visit(function, seen)
                if node['kind'] == 'setVariable':
                    self.assertNotIn('bSelfContext=True', node['properties']['VariableReference'])
        for name in ('GetFloat', 'GetDouble', 'GetInt', 'GetBool', 'GetString'):
            entry = next(n for n in self.graphs[name]['body']['nodes'] if n['kind'] == 'FunctionEntry')
            self.assertTrue(int(entry['properties']['ExtraFlags']) & 0x10000000)
            visit(name, set())

    def test_only_existing_engine_and_file_api_dependencies(self):
        native = codec.native(self.source)
        paths = re.findall(r'/Game/[A-Za-z0-9_/]+', json.dumps(native))
        self.assertEqual(set(paths), {self.source['asset']})
        calls = [n['properties']['FunctionReference'] for g in self.graphs.values()
                 for n in g['body']['nodes'] if n['kind'] == 'call']
        voyage = [c for c in calls if '/Script/Voyage.' in c]
        self.assertEqual(len(voyage), 1)
        self.assertIn('VoyageEditorBlueprintFunctionLibrary', voyage[0])
        self.assertIn('MemberName="LoadFileToArray"', voyage[0])
        self.assertNotIn('IniParserTests', json.dumps(native))


if __name__ == '__main__':
    unittest.main()
