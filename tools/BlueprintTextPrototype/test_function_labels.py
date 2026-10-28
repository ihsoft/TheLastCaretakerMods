"""Offline label identity, inheritance and frozen snapshot checks."""
import unittest

from function_labels import load_labels, lookup, validate
from render_graph import node_lines


class FunctionLabelTests(unittest.TestCase):
    def setUp(self):
        self.catalog = {'functions': {
            '/Script/Engine.Actor::Example': {'title': 'Actor label'},
            '/Script/Engine.KismetMathLibrary::Example': {'title': 'Math label'}},
            'parents': {'/Script/Engine.Pawn': '/Script/Engine.Actor'}}

    def test_exact_owner_and_parent_resolution(self):
        self.assertEqual(lookup({'operation': '/Script/Engine.Pawn::Example'}, catalog=self.catalog)['title'], 'Actor label')
        self.assertEqual(lookup({'operation': '/Script/Engine.KismetMathLibrary::Example'}, catalog=self.catalog)['title'], 'Math label')
        self.assertIsNone(lookup({'operation': '/Script/Voyage.Custom::Example'}, catalog=self.catalog))
        self.assertIsNone(lookup({'operation': 'Example'}, catalog=self.catalog))

    def test_native_reference_and_target_class(self):
        node = {'properties': {'FunctionReference': '(MemberParent="/Script/CoreUObject.Class\'/Script/Engine.Pawn\'",MemberName="Example")'}}
        self.assertEqual(lookup(node, catalog=self.catalog)['title'], 'Actor label')
        node['properties']['FunctionReference'] = '(MemberName="Example",bSelfContext=True)'
        node['pins'] = {'in:target': {'type': 'p1'}}
        templates = {'p1': {'PinType.PinSubCategoryObject': '"/Script/CoreUObject.Class\'/Script/Engine.Pawn\'"'}}
        self.assertEqual(lookup(node, templates, catalog=self.catalog)['title'], 'Actor label')
        self.assertEqual(lookup(node, parent='/Script/Engine.Pawn', catalog=self.catalog)['title'], 'Actor label')
        node['properties']['FunctionReference'] = '(MemberName="Example",MemberGuid=1234,bSelfContext=True)'
        self.assertIsNone(lookup(node, templates, '/Script/Engine.Pawn', self.catalog))

    def test_explicit_blueprint_owner_never_falls_back_to_engine_parent(self):
        node = {'properties': {'FunctionReference': '(MemberParent="/Script/Engine.BlueprintGeneratedClass\'/Game/BP.BP_C\'",MemberName="Example",bSelfContext=True)'}}
        # The type wrapper names an Engine class, but it is not the owning class.
        self.assertIsNone(lookup(node, parent='/Script/Engine.Actor', catalog=self.catalog))

    def test_snapshot_and_renderer_use_engine_labels(self):
        catalog = load_labels()
        self.assertEqual(validate(catalog), catalog)
        cases = [('/Script/Engine.KismetMathLibrary::Add_DoubleDouble', 'float + float'),
                 ('/Script/Engine.KismetMathLibrary::Not_PreBool', 'NOT Boolean'),
                 ('/Script/Engine.KismetSystemLibrary::K2_SetTimerDelegate', 'Set Timer by Event')]
        for operation, title in cases:
            self.assertEqual(node_lines({'id': 'n', 'kind': 'call', 'operation': operation}, set())[0], title)
        self.assertEqual(catalog['functions'][cases[1][0]]['compactTitle'], 'NOT')

    def test_ancestry_cycle_and_unrecognized_function_are_safe(self):
        self.catalog['parents']['/Script/Engine.Actor'] = '/Script/Engine.Pawn'
        self.assertIsNone(lookup({'operation': '/Script/Engine.Pawn::Unknown'}, catalog=self.catalog))


if __name__ == '__main__':
    unittest.main()
