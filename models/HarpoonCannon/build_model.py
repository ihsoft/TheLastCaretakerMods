"""Reproduce the accepted railgun v4 without Unreal, game assets, or old scripts."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import numpy as np
import geometry as core

SOURCE_DIR=Path(__file__).resolve().parent
ROOT=SOURCE_DIR.parents[1]/'artifacts/modeling/HarpoonCannon'
STEM='harpoon_cannon_railgun_c_blockout_v4'

AMMO_CAPACITY = 6

PITCH_PIVOT_X = -18.0

BARREL_LENGTH_SCALE = 1.1

def add_ring_y(scene: core.Scene, name: str, center: tuple[float, float, float], radius: float, depth: float, material: str) -> None:
    cx, cy, cz = center
    scene.add_cylinder_between(name, (cx, cy - depth / 2.0, cz), (cx, cy + depth / 2.0, cz), radius, material, segments=24)

def barrel_x(value: float) -> float:
    return PITCH_PIVOT_X + (value - PITCH_PIVOT_X) * BARREL_LENGTH_SCALE

def barrel_length(value: float) -> float:
    return value * BARREL_LENGTH_SCALE

def _build_assembly(ammo_count: int=AMMO_CAPACITY) -> core.Scene:
    if not 0 <= ammo_count <= AMMO_CAPACITY:
        raise ValueError(f'ammo_count must be 0..{AMMO_CAPACITY}')
    scene = core.Scene()
    scene.add_box('PREVIEW_Deck', (0.0, 0.0, -3.0), (420.0, 330.0, 6.0), 'PreviewDeck', export=False)
    scene.add_z_cylinder('HC_RG_STATIC_DeckFlange', (0.0, 0.0, 6.0), 82.0, 12.0, 'DarkMetal')
    scene.add_torus_z('HC_RG_STATIC_BoltRing', (0.0, 0.0, 12.0), 70.0, 4.0, 'EdgeMetal')
    scene.add_z_cylinder('HC_RG_STATIC_LowerHousing', (0.0, 0.0, 22.0), 72.0, 20.0, 'ServicePanel')
    scene.add_box('HC_RG_STATIC_PowerSocketPlate', (18.0, -72.0, 24.0), (38.0, 8.0, 38.0), 'DarkMetal')
    add_ring_y(scene, 'HC_RG_STATIC_PowerSocketBody', (18.0, -79.0, 24.0), 14.0, 14.0, 'EdgeMetal')
    add_ring_y(scene, 'HC_RG_STATIC_PowerSocketGlow', (18.0, -87.0, 24.0), 11.0, 3.0, 'PowerBlue')
    add_ring_y(scene, 'HC_RG_STATIC_PowerSocketFace', (18.0, -89.0, 24.0), 7.0, 2.0, 'DarkMetal')
    scene.add_cylinder_between('HC_RG_STATIC_InternalPowerConduit', (18.0, -66.0, 24.0), (18.0, -38.0, 32.0), 5.0, 'RubberCable', segments=12)
    scene.add_z_cylinder('HC_RG_YAW_Turntable', (0.0, 0.0, 39.0), 62.0, 18.0, 'RailBlue')
    scene.add_torus_z('HC_RG_YAW_SlipRing', (0.0, 0.0, 48.0), 51.0, 4.5, 'EdgeMetal')
    for side_name, side_y in (('Left', -48.0), ('Right', 48.0)):
        scene.add_box(f'HC_RG_YAW_ElevationFork_{side_name}', (-18.0, side_y, 79.0), (50.0, 16.0, 74.0), 'ServicePanel')
        scene.add_cylinder_between(f'HC_RG_PITCH_Trunnion_{side_name}', (-18.0, side_y - (12.0 if side_y < 0 else -12.0), 107.0), (-18.0, side_y + (10.0 if side_y < 0 else -10.0), 107.0), 16.0, 'EdgeMetal', segments=24)
        add_ring_y(scene, f'HC_RG_YAW_ElevationGear_{side_name}', (-18.0, side_y + (-10.0 if side_y < 0 else 10.0), 107.0), 25.0, 8.0, 'DarkMetal')
    for side_name, side_y in (('Left', -38.0), ('Right', 38.0)):
        scene.add_cylinder_between(f'HC_RG_YAW_AlignmentActuator_{side_name}', (-28.0, side_y, 49.0), (-5.0, side_y, 89.0), 7.0, 'MachinedSteel', segments=16)
    scene.add_box('HC_RG_PITCH_BreechHousing', (-54.0, 0.0, 111.0), (80.0, 58.0, 62.0), 'RailBlue')
    scene.add_box('HC_RG_PITCH_BreechTopPanel', (-54.0, 0.0, 145.0), (64.0, 48.0, 8.0), 'ServicePanel')
    scene.add_box('HC_RG_PITCH_BreechRearLatch', (-96.0, 0.0, 111.0), (8.0, 40.0, 40.0), 'SafetyOrange')
    for side_name, side_y in (('Left', -37.0), ('Right', 37.0)):
        scene.add_cylinder_between(f'HC_RG_PITCH_CapacitorPod_{side_name}', (-102.0, side_y, 118.0), (-30.0, side_y, 118.0), 25.0, 'DarkMetal', segments=24)
        scene.add_cylinder_between(f'HC_RG_PITCH_CapacitorBandRear_{side_name}', (-98.0, side_y, 118.0), (-88.0, side_y, 118.0), 28.0, 'ServicePanel', segments=24)
        scene.add_cylinder_between(f'HC_RG_PITCH_CapacitorBandFront_{side_name}', (-42.0, side_y, 118.0), (-30.0, side_y, 118.0), 28.0, 'ServicePanel', segments=24)
        scene.add_box(f'HC_RG_PITCH_CapacitorWindow_{side_name}', (-65.0, side_y + (-25.5 if side_y < 0 else 25.5), 118.0), (30.0, 3.0, 12.0), 'CopperCoil')
    scene.add_box('HC_RG_PITCH_ChannelFloor', (barrel_x(67.0), 0.0, 99.0), (barrel_length(190.0), 18.0, 6.0), 'DarkMetal')
    scene.add_box('HC_RG_PITCH_RailLeft', (barrel_x(67.0), -8.0, 111.0), (barrel_length(190.0), 6.0, 14.0), 'MachinedSteel')
    scene.add_box('HC_RG_PITCH_RailRight', (barrel_x(67.0), 8.0, 111.0), (barrel_length(190.0), 6.0, 14.0), 'MachinedSteel')
    module_centers = (-2.0, 34.0, 70.0, 106.0, 142.0)
    for index, original_x in enumerate(module_centers, start=1):
        x_pos = barrel_x(original_x)
        scene.add_box(f'HC_RG_PITCH_CoilModuleTop_{index:02d}', (x_pos, 0.0, 127.0), (barrel_length(24.0), 32.0, 8.0), 'RailBlue')
        scene.add_box(f'HC_RG_PITCH_CoilWindowLeft_{index:02d}', (x_pos, -17.0, 111.0), (barrel_length(18.0), 3.0, 10.0), 'CopperCoil')
        scene.add_box(f'HC_RG_PITCH_CoilWindowRight_{index:02d}', (x_pos, 17.0, 111.0), (barrel_length(18.0), 3.0, 10.0), 'CopperCoil')
        scene.add_box(f'HC_RG_PITCH_CoilStatus_{index:02d}', (x_pos, -19.0, 125.0), (10.0, 2.0, 4.0), 'PowerBlue')
    muzzle_x = barrel_x(166.0)
    scene.add_box('HC_RG_PITCH_MuzzleTop', (muzzle_x, 0.0, 132.0), (12.0, 40.0, 8.0), 'SafetyOrange')
    scene.add_box('HC_RG_PITCH_MuzzleBottom', (muzzle_x, 0.0, 94.0), (12.0, 40.0, 8.0), 'SafetyOrange')
    scene.add_box('HC_RG_PITCH_MuzzleLeft', (muzzle_x, -16.0, 113.0), (12.0, 8.0, 30.0), 'SafetyOrange')
    scene.add_box('HC_RG_PITCH_MuzzleRight', (muzzle_x, 16.0, 113.0), (12.0, 8.0, 30.0), 'SafetyOrange')
    scene.add_cylinder_between('HC_RG_PITCH_ChamberedRod', (-84.0, 0.0, 111.0), (barrel_x(145.0), 0.0, 111.0), 4.5, 'MachinedSteel', segments=16)
    scene.add_box('HC_RG_YAW_AmmoBasketBottom', (-67.0, -86.0, 47.0), (100.0, 26.0, 8.0), 'DarkMetal')
    scene.add_box('HC_RG_YAW_AmmoBasketFrontRailLow', (-67.0, -99.0, 75.0), (100.0, 5.0, 8.0), 'EdgeMetal')
    scene.add_box('HC_RG_YAW_AmmoBasketFrontRailHigh', (-67.0, -99.0, 105.0), (100.0, 5.0, 8.0), 'EdgeMetal')
    scene.add_box('HC_RG_YAW_AmmoBasketRearRailLow', (-67.0, -73.0, 75.0), (100.0, 5.0, 8.0), 'EdgeMetal')
    scene.add_box('HC_RG_YAW_AmmoBasketRearRailHigh', (-67.0, -73.0, 105.0), (100.0, 5.0, 8.0), 'EdgeMetal')
    scene.add_box('HC_RG_YAW_AmmoBasketRearGuard', (-116.0, -86.0, 86.0), (8.0, 26.0, 86.0), 'SafetyOrange')
    scene.add_box('HC_RG_YAW_AmmoBasketFrontGuard', (-18.0, -86.0, 86.0), (8.0, 26.0, 86.0), 'SafetyOrange')
    scene.add_box('HC_RG_YAW_AmmoBasketRearHandle', (-116.0, -86.0, 132.0), (8.0, 26.0, 8.0), 'SafetyOrange')
    scene.add_box('HC_RG_YAW_AmmoBasketFrontHandle', (-18.0, -86.0, 132.0), (8.0, 26.0, 8.0), 'SafetyOrange')
    for index in range(AMMO_CAPACITY):
        x_pos = -106.0 + index * 15.5
        scene.add_box(f'HC_RG_YAW_AmmoCup_{index + 1:02d}', (x_pos, -86.0, 54.0), (12.0, 20.0, 12.0), 'RailBlue')
        scene.add_torus_z(f'HC_RG_YAW_AmmoGuide_{index + 1:02d}', (x_pos, -86.0, 87.0), 5.8, 1.4, 'EdgeMetal', major_segments=16, minor_segments=8)
        if index < ammo_count:
            scene.add_cylinder_between(f'HC_RG_AMMO_Rod_{index + 1:02d}', (x_pos, -86.0, 58.0), (x_pos, -86.0, 126.0), 4.5, 'MachinedSteel', segments=16)
    scene.add_box('HC_RG_PITCH_OpticHousing', (-27.0, -40.0, 158.0), (28.0, 14.0, 18.0), 'DarkMetal')
    scene.add_box('HC_RG_PITCH_OpticGlass', (-12.0, -40.0, 158.0), (3.0, 10.0, 12.0), 'PowerBlue')
    scene.add_cylinder_between('HC_RG_YAW_ControlGrip', (-8.0, -58.0, 86.0), (-8.0, -71.0, 69.0), 4.5, 'RubberCable', segments=14)
    return scene

def scene_stats(scene: core.Scene) -> dict[str, object]:
    exported = [obj for obj in scene.objects if obj.export]
    vertices = [vertex for obj in exported for vertex in obj.vertices]
    bounds_min = np.min(np.array(vertices), axis=0)
    bounds_max = np.max(np.array(vertices), axis=0)
    return {'objectCount': len(exported), 'vertexCount': sum((len(obj.vertices) for obj in exported)), 'faceCount': sum((len(obj.faces) for obj in exported)), 'boundsMinCm': bounds_min.round(3).tolist(), 'boundsMaxCm': bounds_max.round(3).tolist(), 'sizeCm': (bounds_max - bounds_min).round(3).tolist(), 'ammoCapacity': AMMO_CAPACITY, 'ammoObjectNames': [f'HC_RG_AMMO_Rod_{index:02d}' for index in range(1, AMMO_CAPACITY + 1)], 'powerSocketPlaceholder': 'HC_RG_STATIC_PowerSocket*', 'integrationStatus': 'source-only; not game-ready or runtime-validated'}

def spike(name, start, shoulder, tip):
    scene = core.Scene()
    scene.add_cylinder_between(name, start, shoulder, 4.5, 'MachinedSteel', segments=24)
    obj = scene.objects[0]
    obj.faces.pop()
    tip_index = len(obj.vertices)
    obj.vertices.append(np.array(tip, dtype=float))
    for i in range(24):
        obj.faces.append((24 + i, 24 + (i + 1) % 24, tip_index))
    return obj

def _fit_assembly():
    scene = _build_assembly()
    remove = {'HC_RG_PITCH_ChamberedRod', 'HC_RG_YAW_ControlGrip'}
    scene.objects = [o for o in scene.objects if o.export and o.name not in remove and (not o.name.startswith('HC_RG_AMMO_Rod_')) and (not o.name.startswith('HC_RG_PITCH_Muzzle')) and (not o.name.startswith('HC_RG_YAW_AmmoBasketRearHandle')) and (not o.name.startswith('HC_RG_YAW_AmmoBasketFrontHandle')) and (not o.name.startswith('HC_RG_YAW_AmmoBasketRearGuard')) and (not o.name.startswith('HC_RG_YAW_AmmoBasketFrontGuard'))]
    for o in scene.objects:
        if 'CoilWindow' in o.name:
            o.material = 'SafetyOrange'
        if 'CoilModuleTop_' in o.name or 'CoilWindow' in o.name or 'CoilStatus_' in o.name:
            station = int(o.name.rsplit('_', 1)[1]) - 1
            new_x = 20.0 + station * 34.5
            delta = new_x - float(np.mean(np.array(o.vertices)[:, 0]))
            for p in o.vertices:
                p[0] += delta
        if 'CapacitorBandFront_' in o.name:
            for p in o.vertices:
                if abs(p[0] + 30) < 1e-06:
                    p[0] -= 1
        if o.name == 'HC_RG_YAW_AmmoBasketBottom':
            for p in o.vertices:
                p[0] = -67 + (p[0] + 67) * 0.9
    for side, x in [('Rear', -116), ('Front', -18)]:
        scene.add_box(f'HC_RG_YAW_AmmoBasket{side}Guard', (x, -86, 89.5), (8, 26, 93), 'SafetyOrange')
    mx = barrel_x(166)
    for name, y, z, sy, sz in [('Top', 0, 127, 40, 8), ('Bottom', 0, 95, 40, 8), ('Left', -16, 111, 8, 24), ('Right', 16, 111, 8, 24)]:
        scene.add_box('HC_RG_PITCH_Muzzle' + name, (mx, y, z), (12, sy, sz), 'SafetyOrange')
    for i in range(6):
        x = -106 + i * 15.5
        scene.objects.append(spike(f'HC_RG_AMMO_Rod_{i + 1:02}', (x, -86, 58), (x, -86, 126), (x, -86, 144)))
    scene.objects.append(spike('HC_RG_PITCH_ChamberedRod', (-84, 0, 111), (-16, 0, 111), (2, 0, 111)))
    for o in scene.objects:
        used = sorted({i for f in o.faces for i in f})
        remap = {old: new for new, old in enumerate(used)}
        o.vertices = [o.vertices[i] for i in used]
        o.faces = [tuple((remap[i] for i in f)) for f in o.faces]
    return scene

def _audit_solids(scene):
    reports = []
    for o in scene.objects:
        edges, directed = (Counter(), Counter())
        volume = 0.0
        max_planarity = 0.0
        min_area = float('inf')
        for face in o.faces:
            p = np.array([o.vertices[i] for i in face])
            n = np.cross(p[1] - p[0], p[2] - p[0])
            n /= np.linalg.norm(n)
            max_planarity = max(max_planarity, float(np.max(np.abs((p - p[0]) @ n))))
            for a, b in zip(face, face[1:] + face[:1]):
                edges[tuple(sorted((a, b)))] += 1
                directed[a, b] += 1
            for tri in core.triangulate(face):
                a, b, c = [o.vertices[i] for i in tri]
                min_area = min(min_area, float(np.linalg.norm(np.cross(b - a, c - a)) / 2))
                volume += float(np.dot(a, np.cross(b, c)) / 6)
        bad_edges = sum((count != 2 for count in edges.values()))
        bad_winding = sum((directed[a, b] != directed[b, a] for a, b in edges))
        box = len(o.vertices) == 8 and len(o.faces) == 6
        box_exact = all((len(set(np.round(np.array(o.vertices)[:, axis], 8))) == 2 for axis in range(3))) if box else None
        assert bad_edges == bad_winding == 0, (o.name, bad_edges, bad_winding)
        assert volume > 0 and min_area > 1e-07 and (max_planarity < 1e-06), (o.name, volume, min_area, max_planarity)
        assert box_exact is not False, o.name
        reports.append(dict(name=o.name, closed=True, outwardWinding=True, volumeCm3=round(volume, 4), planarErrorCm=max_planarity, axisAlignedBox=box_exact))
    by_name = {o.name: o for o in scene.objects}
    centers = []
    for i in range(1, 6):
        names = [f'HC_RG_PITCH_{part}_{i:02}' for part in ['CoilModuleTop', 'CoilWindowLeft', 'CoilWindowRight']]
        xs = [float(np.mean(np.array(by_name[n].vertices)[:, 0])) for n in names]
        assert max(xs) - min(xs) < 1e-08
        centers.append(xs[0])
    assert len([o for o in scene.objects if 'CoilModuleTop_' in o.name]) == 5
    assert len([o for o in scene.objects if 'CoilWindowLeft_' in o.name]) == 5
    assert len([o for o in scene.objects if 'CoilWindowRight_' in o.name]) == 5
    return dict(objects=reports, moduleCentersXCm=centers, topLeftRightCounts=[5, 5, 5], ammunitionCount=6, rodDiameterCm=9, rodLengthCm=86, tipLengthCm=18, railClearWidthCm=10, muzzleOuterYZCm=[40, 40], notes='Per-object closed solids; intentional assembly intersections are not boolean-unioned. UVs, final materials, UE import and runtime pending.')

def export(scene):
    lines = [f'mtllib {STEM}.mtl', '# Source-only mesh; units centimeters; +X firing, +Z up']
    vi, ni = (1, 1)
    for o in scene.objects:
        lines += ['o ' + o.name, 'usemtl ' + o.material, 's off']
        lines += ['v ' + ' '.join((f'{v:.8f}' for v in p)) for p in o.vertices]
        for face in o.faces:
            for tri in core.triangulate(face):
                a, b, c = [o.vertices[i] for i in tri]
                normal = np.cross(b - a, c - a)
                normal /= np.linalg.norm(normal)
                lines.append('vn ' + ' '.join((f'{v:.8f}' for v in normal)))
                lines.append('f ' + ' '.join((f'{vi + i}//{ni}' for i in tri)))
                ni += 1
        vi += len(o.vertices)
    path = ROOT / (STEM + '.obj')
    path.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    mats = []
    for name, color in core.MATERIALS.items():
        mats += ['newmtl ' + name, 'Kd ' + ' '.join((str(v) for v in color)), 'Ks 0.2 0.2 0.2', 'Ns 50', '']
    (ROOT / (STEM + '.mtl')).write_text('\n'.join(mats), encoding='utf-8')
    return path

def read_obj(path):
    scene = core.Scene()
    vertices = []
    indices = {}
    obj = None
    for line in path.read_text(encoding='utf-8').splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == 'o':
            obj = core.MeshObject(parts[1], '', [], [])
            scene.objects.append(obj)
            indices = {}
        elif parts[0] == 'usemtl':
            obj.material = parts[1]
        elif parts[0] == 'v':
            vertices.append(np.array([float(x) for x in parts[1:4]]))
        elif parts[0] == 'f':
            face = []
            for item in parts[1:]:
                i = int(item.split('/')[0]) - 1
                if i not in indices:
                    indices[i] = len(obj.vertices)
                    obj.vertices.append(vertices[i])
                face.append(indices[i])
            obj.faces.append(tuple(face))
    return scene

def outline(half_y, half_z, bevel):
    return [(-half_y + bevel, -half_z), (half_y - bevel, -half_z), (half_y, -half_z + bevel), (half_y, half_z - bevel), (half_y - bevel, half_z), (-half_y + bevel, half_z), (-half_y, half_z - bevel), (-half_y, -half_z + bevel)]

def muzzle():
    vertices = []
    outer = [(174, 18, 18, 4), (187.4, 18, 18, 4), (190.4, 15, 15, 3)]
    for x, hy, hz, b in outer:
        vertices += [np.array([x, y, 111 + z]) for y, z in outline(hy, hz, b)]
    for x in (174, 190.4):
        vertices += [np.array([x, y, 111 + z]) for y, z in outline(5, 5, 0.5)]
    faces = []
    for layer in range(2):
        for i in range(8):
            j = (i + 1) % 8
            faces.append((layer * 8 + i, layer * 8 + j, (layer + 1) * 8 + j, (layer + 1) * 8 + i))
    for i in range(8):
        j = (i + 1) % 8
        faces += [(24 + i, 32 + i, 32 + j, 24 + j), (i, 24 + i, 24 + j, j), (16 + i, 16 + j, 32 + j, 32 + i)]
    return core.MeshObject('HC_RG_PITCH_MuzzleShroud', 'DarkMetal', vertices, faces)

def cheek(side):
    verts = [np.array([x, side * 18.5 + y, 111 + z]) for x in (179, 186) for y, z in outline(1.5, 15, 1)]
    faces = [tuple(range(7, -1, -1)), tuple(range(8, 16))]
    for i in range(8):
        j = (i + 1) % 8
        faces.append((i, j, 8 + j, 8 + i))
    return core.MeshObject('HC_RG_PITCH_MuzzleGuard' + ('Left' if side < 0 else 'Right'), 'SafetyOrange', verts, faces)

def build_scene():
    scene = _fit_assembly()
    scene.objects = [o for o in scene.objects if not o.name.startswith('HC_RG_PITCH_Muzzle') and (not o.name.startswith('HC_RG_STATIC_PowerSocket')) and (o.name != 'HC_RG_STATIC_InternalPowerConduit')]
    for o in scene.objects:
        if o.name in ['HC_RG_PITCH_RailLeft', 'HC_RG_PITCH_RailRight']:
            max_x = max((p[0] for p in o.vertices))
            for p in o.vertices:
                if abs(p[0] - max_x) < 1e-06:
                    p[0] = 174
    scene.objects += [muzzle(), cheek(-1), cheek(1)]
    scene.add_box('HC_RG_STATIC_PowerSocketPlate', (0, -69, 22), (34, 10, 28), 'DarkMetal')
    for suffix, y, r, depth, material in [('Body', -76, 11, 10, 'EdgeMetal'), ('Glow', -81.5, 8.5, 2, 'PowerBlue'), ('Face', -83, 5.5, 2, 'DarkMetal')]:
        scene.add_cylinder_between('HC_RG_STATIC_PowerSocket' + suffix, (0, y - depth / 2, 22), (0, y + depth / 2, 22), r, material, segments=32)
    scene.add_cylinder_between('HC_RG_STATIC_InternalPowerConduit', (0, -64, 22), (0, -38, 28), 5, 'RubberCable', segments=12)
    for o in scene.objects:
        used = sorted({i for f in o.faces for i in f})
        remap = {old: new for new, old in enumerate(used)}
        o.vertices = [o.vertices[i] for i in used]
        o.faces = [tuple((remap[i] for i in f)) for f in o.faces]
    return scene

def audit(scene):
    r = _audit_solids(scene)
    r.pop('muzzleOuterYZCm')
    r.update(muzzleOuterYZCm=[36, 36], muzzleNoseYZCm=[30, 30], muzzleBoreYZCm=[10, 10], powerInletCenterXZCm=[0, 22], powerMountBackYCm=-64, powerMountAttachment='plate overlaps fixed lower housing; body overlaps plate')
    by_name = {o.name: o for o in scene.objects}
    for part in ['Plate', 'Body', 'Glow', 'Face']:
        p = np.array(by_name['HC_RG_STATIC_PowerSocket' + part].vertices)
        center = (p.min(axis=0) + p.max(axis=0)) / 2
        assert abs(center[0]) < 1e-08 and abs(center[2] - 22) < 1e-08
    assert 17 ** 2 + 64 ** 2 < 72 ** 2
    assert -81 < -74 < -71
    return r

def main():
    global ROOT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT)
    args=parser.parse_args()
    ROOT=args.output.resolve()
    if ROOT==SOURCE_DIR or SOURCE_DIR in ROOT.parents:
        parser.error('Generated output must not overwrite the accepted source directory')
    ROOT.mkdir(parents=True,exist_ok=True)
    scene=build_scene()
    report=audit(scene)
    obj=export(scene)
    report['exportReadback']=audit(read_obj(obj))
    report['stats']=scene_stats(scene)
    receipt=json.loads((SOURCE_DIR/'baseline.json').read_text(encoding='utf-8'))
    checks={}
    for filename,expected in receipt['sha256'].items():
        actual=hashlib.sha256((ROOT/filename).read_bytes()).hexdigest()
        checks[filename]=actual
        if actual.upper()!=expected.upper():
            raise RuntimeError('Accepted baseline hash mismatch: '+filename)
    report['sha256']=checks
    (ROOT/(STEM+'_audit.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'result':'passed','output':str(ROOT),'objects':len(scene.objects),'baselineHashes':'match'}))


if __name__=='__main__': main()
