"""Reviewed UE expression signatures. This is a presentation allowlist, not reflection."""


def native_class(name):
    return '"/Script/CoreUObject.Class\'/Script/Engine.' + name + '\'"'


OPERATIONS = {}
METHODS = {}
CONTAINERS = {}
LABELS = {}


def operation(owner, name, symbol, names, types, result, labels=None):
    OPERATIONS[native_class(owner), name] = (symbol, tuple(names.split()), tuple(types.split()), result)
    if labels:
        LABELS[symbol] = labels


for suffix, kind in [('IntInt', 'int'), ('BoolBool', 'bool'), ('ByteByte', 'byte'),
                     ('ObjectObject', 'object'), ('ClassClass', 'class'), ('NameName', 'name')]:
    comparisons = [('EqualEqual', '=='), ('NotEqual', '!=')]
    if kind in ('int', 'byte'):
        comparisons += [('Less', '<'), ('LessEqual', '<='), ('Greater', '>'), ('GreaterEqual', '>=')]
    for name, symbol in comparisons:
        operation('KismetMathLibrary', name + '_' + suffix, symbol, 'A B', kind + ' ' + kind, 'bool')
for name, symbol in [('Add', '+'), ('Subtract', '-'), ('Multiply', '*'),
                     ('Divide', 'intDivide'), ('Percent', 'intRemainder')]:
    operation('KismetMathLibrary', name + '_IntInt', symbol, 'A B', 'int int', 'int')
for name, symbol, args, types, result in [
    ('Abs', 'abs', 'A', 'double', 'double'), ('Abs_Int', 'abs', 'A', 'int', 'int'),
    ('Min', 'min', 'A B', 'int int', 'int'), ('Max', 'max', 'A B', 'int int', 'int'),
    ('Clamp', 'clamp', 'Value Min Max', 'int int int', 'int'),
    ('FClamp', 'clamp', 'Value Min Max', 'double double double', 'double'),
    ('Round', 'round', 'A', 'double', 'int'), ('FFloor', 'floor', 'A', 'double', 'int'),
    ('FCeil', 'ceil', 'A', 'double', 'int'), ('FTrunc', 'truncate', 'A', 'double', 'int'),
    ('Conv_IntToDouble', 'double', 'InInt', 'int', 'double'),
    ('Conv_FloatToDouble', 'double', 'InFloat', 'float', 'double'),
    ('Conv_DoubleToFloat', 'float', 'InDouble', 'double', 'float'),
    ('SelectInt', 'select', 'bPickA A B', 'bool int int', 'int'),
    ('SelectString', 'select', 'bPickA A B', 'bool string string', 'string'),
    ('VSize', 'length', 'A', 'Vector', 'double'),
    ('VSizeSquared', 'lengthSquared', 'A', 'Vector', 'double'),
    ('Vector_Distance', 'distance', 'V1 V2', 'Vector Vector', 'double'),
    ('VLerp', 'lerp', 'A B Alpha', 'Vector Vector float', 'Vector'),
    ('ClassIsChildOf', 'ClassIsChildOf', 'TestClass ParentClass', 'class class', 'bool'),
]:
    operation('KismetMathLibrary', name, symbol, args, types, result)
for name, symbol in [('Normal', 'normalize'), ('Vector_Normal2D', 'normalize2D'),
                     ('Vector_IsNearlyZero', 'isNearlyZero')]:
    operation('KismetMathLibrary', name, symbol, 'A Tolerance', 'Vector float',
              'bool' if name == 'Vector_IsNearlyZero' else 'Vector', ('', 'tolerance: '))
operation('KismetMathLibrary', 'NearlyEqual_FloatFloat', 'nearlyEqual', 'A B ErrorTolerance',
          'double double double', 'bool', ('', '', 'tolerance: '))
for name, kind in [('InRange_IntInt', 'int'), ('InRange_FloatFloat', 'double')]:
    operation('KismetMathLibrary', name, 'inRange', 'Value Min Max InclusiveMin InclusiveMax',
              f'{kind} {kind} {kind} bool bool', 'bool', ('', 'min: ', 'max: ', 'inclusiveMin: ', 'inclusiveMax: '))

operation('KismetSystemLibrary', 'IsValid', 'IsValid', 'Object', 'object', 'bool')
operation('KismetSystemLibrary', 'IsValidClass', 'IsValidClass', 'Class', 'class', 'bool')
for name, symbol in [('EqualEqual_StrStr', 'equalsExact'), ('NotEqual_StrStr', 'notEqualsExact'),
                     ('EqualEqual_StriStri', 'equalsIgnoreCase'), ('NotEqual_StriStri', 'notEqualsIgnoreCase')]:
    operation('KismetStringLibrary', name, symbol, 'A B', 'string string', 'bool')
for name, symbol, arg, result in [('Len', 'length', 'S', 'int'),
                                  ('IsEmpty', 'isEmpty', 'InString', 'bool'),
                                  ('IsNumeric', 'isNumeric', 'SourceString', 'bool')]:
    operation('KismetStringLibrary', name, symbol, arg, 'string', result)
operation('KismetStringLibrary', 'Contains', 'containsText', 'SearchIn Substring bUseCase bSearchFromEnd',
          'string string bool bool', 'bool', ('', '', 'caseSensitive: ', 'fromEnd: '))
for name, arg in [('StartsWith', 'InPrefix'), ('EndsWith', 'InSuffix')]:
    operation('KismetStringLibrary', name, name[0].lower() + name[1:], f'SourceString {arg} SearchCase',
              'string string SearchCase', 'bool', ('', '', 'case: '))
operation('KismetTextLibrary', 'TextIsEmpty', 'textIsEmpty', 'InText', 'text', 'bool')

# A method's receiver is an operand. An implicit Self still appears as Self.
for owner, name, args, types in [
    ('Pawn', 'IsPlayerControlled', '', ''), ('Actor', 'HasAuthority', '', ''),
    ('Controller', 'IsLocalController', '', ''), ('Actor', 'ActorHasTag', 'Tag', 'name'),
    ('ActorComponent', 'ComponentHasTag', 'Tag', 'name'),
    ('SceneComponent', 'DoesSocketExist', 'InSocketName', 'name'),
]:
    METHODS[native_class(owner), name] = ('@' + name, tuple(('self ' + args).split()),
                                         tuple(('object ' + types).split()), 'bool')

# Wildcard containers must be specialized in the actual pins, including map values.
for owner, prefix, kind, target, item in [
    ('KismetArrayLibrary', 'Array', 'Array', 'TargetArray', 'ItemToFind'),
    ('BlueprintSetLibrary', 'Set', 'Set', 'TargetSet', 'ItemToFind'),
    ('BlueprintMapLibrary', 'Map', 'Map', 'TargetMap', 'Key'),
]:
    for name, symbol, extra, result in [('Length', 'count', (), 'int'),
                                       ('IsEmpty', 'isEmpty', (), 'bool'),
                                       ('Contains', 'containsKey' if kind == 'Map' else 'contains', (item,), 'bool')]:
        CONTAINERS[native_class(owner), prefix + '_' + name] = (symbol, (target,) + extra, kind, result)
CONTAINERS[native_class('KismetArrayLibrary'), 'Array_IsValidIndex'] = (
    'isValidIndex', ('TargetArray', 'IndexToTest'), 'Array', 'bool')

INFIX = {'+', '-', '*', '/', '<', '<=', '>', '>=', '==', '!=', 'AND', 'OR', 'XOR'}
