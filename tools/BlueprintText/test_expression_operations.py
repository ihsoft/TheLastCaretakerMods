"""Semantic boundaries for predicate, container and numeric expression previews."""
import copy
import unittest

from expression_operations import native_class
from graph_expressions import format_expression, flat_expression
from test_graph_expressions import TYPES, call, read, folded


def native(owner, name, args, result='bool', defaults=None, self_context=False):
    node = call('op', name, args, result, defaults)
    node['properties']['FunctionReference'] = (
        '(MemberParent=' + native_class(owner) + ',MemberName="' + name + '"'
        + (',bSelfContext=True' if self_context else '') + ')')
    return node


def document(node):
    types = copy.deepcopy(TYPES)
    types.update({k: {'PinType.PinCategory': '"'+k+'"'}
                  for k in ('object','class','name','int','byte','string','text')})
    types['case'] = {'PinType.PinCategory':'"byte"', 'PinType.PinSubCategoryObject':
                    '"/Script/CoreUObject.Enum\'/Script/CoreUObject.ESearchCase\'"'}
    return dict(schema=2, pinTemplates=types, nodes=[node], links=[])


def connect(doc, name, kind, pin):
    nid = 'read' + str(len(doc['nodes']))
    doc['nodes'].append(read(nid, name, kind))
    doc['links'].append({'from':nid+'.out:result', 'to':'op.in:'+pin})


def expression(doc):
    return next(n for n in folded(doc)[0] if n['id']=='op').get('_formula')


class OperationTests(unittest.TestCase):
    def test_validity_keeps_native_semantics_source_and_owner(self):
        doc = document(native('KismetSystemLibrary','IsValid',[('Object','object')]))
        connect(doc, 'Target', 'object', 'Object')
        original = copy.deepcopy(doc)
        self.assertEqual(expression(doc), 'IsValid(Target)')
        self.assertEqual(doc, original)
        doc['nodes'][0]['properties']['FunctionReference'] = '(MemberParent='+native_class('Actor')+',MemberName="IsValid")'
        self.assertIsNone(expression(doc))
        doc['nodes'][0] = native('KismetMathLibrary','NotEqual_ObjectObject',
            [('A','object'),('B','object')], defaults={'in:B':'None'})
        doc['links'][0]['to']='op.in:A'
        self.assertEqual(expression(doc), '(Target != null)')

    def test_methods_keep_receiver_and_do_not_invent_external_self(self):
        doc = document(native('Actor','ActorHasTag',[('self','object'),('Tag','name')],
                              defaults={'in:Tag':'Enabled'}, self_context=True))
        self.assertEqual(expression(doc), 'Self.ActorHasTag(Name("Enabled"))')
        connect(doc, 'TargetActor', 'object', 'self')
        self.assertEqual(expression(doc), 'TargetActor.ActorHasTag(Name("Enabled"))')
        doc['links']=[]
        doc['nodes'][0]['properties']['FunctionReference']=doc['nodes'][0]['properties']['FunctionReference'].replace(',bSelfContext=True','')
        self.assertIsNone(expression(doc))
        doc = document(native('Actor','HasAuthority',[('self','object')], self_context=True))
        node=doc['nodes'][0]
        node['properties']['FunctionReference']='(MemberName="HasAuthority",bSelfContext=True)'
        doc['pinTemplates']['object']['PinType.PinSubCategoryObject']=native_class('Actor')
        self.assertEqual(expression(doc), 'Self.HasAuthority()')
        node['properties']['FunctionReference']='(MemberName="HasAuthority",MemberGuid=ABC,bSelfContext=True)'
        self.assertIsNone(expression(doc))

    def test_string_modes_flags_and_empty_literal_are_not_lost(self):
        for member, symbol in [('EqualEqual_StrStr','equalsExact'), ('EqualEqual_StriStri','equalsIgnoreCase')]:
            doc=document(native('KismetStringLibrary',member,[('A','string'),('B','string')],
                                defaults={'in:A':'a"b','in:B':''}))
            self.assertEqual(expression(doc), symbol+'("a\\"b", "")')
            del doc['nodes'][0]['defaults']['in:B']
            self.assertIsNone(expression(doc))  # Missing is not an explicit empty string.
        doc=document(native('KismetStringLibrary','Contains',
            [('SearchIn','string'),('Substring','string'),('bUseCase','bool'),('bSearchFromEnd','bool')],
            defaults={'in:SearchIn':'ABC','in:Substring':'a','in:bUseCase':'false','in:bSearchFromEnd':'true'}))
        self.assertEqual(expression(doc), 'containsText("ABC", "a", caseSensitive: false, fromEnd: true)')
        doc=document(native('KismetStringLibrary','StartsWith',
            [('SourceString','string'),('InPrefix','string'),('SearchCase','case')],
            defaults={'in:SourceString':'ABC','in:InPrefix':'a','in:SearchCase':'IgnoreCase'}))
        self.assertEqual(expression(doc), 'startsWith("ABC", "a", case: ESearchCase.IgnoreCase)')

    def test_integer_division_and_enum_conversion_are_explicit(self):
        doc=document(native('KismetMathLibrary','Divide_IntInt',[('A','int'),('B','int')],
                            'int', {'in:A':'5','in:B':'2'}))
        self.assertEqual(expression(doc), 'intDivide(5, 2)')
        doc=document(native('KismetMathLibrary','EqualEqual_ByteByte',[('A','byte'),('B','byte')],
                            defaults={'in:B':'0'}))
        connect(doc, 'Mode', 'case', 'A')
        self.assertEqual(expression(doc), '(byte(Mode) == 0)')
        doc['nodes'][0]['pins']['in:B']['type']='case'
        self.assertIsNone(expression(doc))  # A mismatched declared signature is not inferred.

    def test_vector_tolerance_and_range_inclusivity_are_preserved(self):
        doc=document(native('KismetMathLibrary','Vector_IsNearlyZero',[('A','Vector'),('Tolerance','float')],
                            defaults={'in:Tolerance':'0.01'}))
        connect(doc,'Velocity','Vector','A')
        doc['pinTemplates']['constVector']=dict(doc['pinTemplates']['Vector'],
            **{'PinType.bIsReference':'True','PinType.bIsConst':'True'})
        doc['nodes'][0]['pins']['in:A']['type']='constVector'
        self.assertEqual(expression(doc), 'isNearlyZero(Velocity, tolerance: 0.01)')
        doc['pinTemplates']['constVector']['PinType.bIsConst']='False'
        self.assertIsNone(expression(doc))
        doc=document(native('KismetMathLibrary','InRange_IntInt',
            [('Value','int'),('Min','int'),('Max','int'),('InclusiveMin','bool'),('InclusiveMax','bool')],
            defaults={'in:Value':'10','in:Min':'0','in:Max':'10','in:InclusiveMin':'true','in:InclusiveMax':'false'}))
        self.assertEqual(expression(doc), 'inRange(10, min: 0, max: 10, inclusiveMin: true, inclusiveMax: false)')

    def test_concrete_container_keys_values_and_reference_boundaries(self):
        for owner, prefix, kind, target, key in [
            ('KismetArrayLibrary','Array','Array','TargetArray','ItemToFind'),
            ('BlueprintSetLibrary','Set','Set','TargetSet','ItemToFind'),
            ('BlueprintMapLibrary','Map','Map','TargetMap','Key')]:
            with self.subTest(kind=kind):
                doc=document(native(owner,prefix+'_Contains',[(target,'container'),(key,'int')],defaults={'in:'+key:'2'}))
                metadata=dict(doc['pinTemplates']['int'], **{'PinType.ContainerType':kind,
                    'PinType.PinValueType':'(TerminalCategory="string")' if kind=='Map' else '()'})
                doc['pinTemplates']['container']=metadata
                doc['pinTemplates']['constContainer']=dict(metadata, **{'PinType.bIsReference':'True','PinType.bIsConst':'True'})
                doc['nodes'][0]['pins']['in:'+target]['type']='constContainer'
                connect(doc,'Items','container',target)
                self.assertEqual(expression(doc), ('containsKey' if kind=='Map' else 'contains')+'(Items, 2)')
                doc['pinTemplates']['constContainer']['PinType.bIsConst']='False'
                self.assertIsNone(expression(doc))
                doc['pinTemplates']['constContainer']['PinType.bIsConst']='True'
                if kind=='Map':
                    metadata['PinType.PinValueType']='(TerminalCategory="int")'
                    self.assertIsNone(expression(doc))
                else:
                    doc['nodes'][0]['pins']['in:'+key]['type']='byte'
                    self.assertIsNone(expression(doc))

    def test_unknown_stateful_and_multi_output_calls_stay_nodes(self):
        for owner,name in [('KismetMathLibrary','RandomFloat'), ('KismetSystemLibrary','GetGameTimeInSeconds'),
                           ('BlueprintMapLibrary','Map_Find')]:
            doc=document(native(owner,name,[],result='double'))
            self.assertIsNone(expression(doc))
        doc=document(native('KismetSystemLibrary','IsValid',[('Object','object')],defaults={'in:Object':'None'}))
        doc['nodes'][0]['pins']['out:Other']={'type':'bool'}
        self.assertIsNone(expression(doc))

    def test_wrapping_preserves_named_flags_and_method_receiver(self):
        for tree in [
            {'op':'@ActorHasTag','args':['VeryLongActorReceiverName','Name("SomeTag")']},
            {'op':'containsText','args':['SourceText','"abc"','false','true']}]:
            self.assertEqual(''.join(''.join(format_expression(tree,width=25)).removeprefix('Output:').split()),
                             ''.join(flat_expression(tree).split()))


if __name__ == '__main__':
    unittest.main()
