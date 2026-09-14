"""Authored mesh primitives and placeholder material palette; centimeters."""
import math
from dataclasses import dataclass
import numpy as np

MATERIALS = {'DarkMetal': (0.16, 0.185, 0.195), 'EdgeMetal': (0.32, 0.35, 0.36), 'TealPaint': (0.065, 0.33, 0.32), 'RubberCable': (0.11, 0.13, 0.135), 'AmberLight': (0.95, 0.36, 0.055), 'ConnectorProxy': (0.76, 0.31, 0.07), 'CableGuide': (0.07, 0.3, 0.32), 'PreviewDeck': (0.055, 0.19, 0.19)}

@dataclass
class MeshObject:
    name: str
    material: str
    vertices: list[np.ndarray]
    faces: list[tuple[int, ...]]
    export: bool = True

class Scene:

    def __init__(self) -> None:
        self.objects: list[MeshObject] = []

    def add_box(self, name: str, center: tuple[float, float, float], size: tuple[float, float, float], material: str, export: bool=True) -> None:
        cx, cy, cz = center
        sx, sy, sz = (v / 2.0 for v in size)
        vertices = [np.array([cx + x * sx, cy + y * sy, cz + z * sz], dtype=float) for x, y, z in ((-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1), (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1))]
        faces = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        self.objects.append(MeshObject(name, material, vertices, faces, export))

    def add_cylinder_between(self, name: str, start: tuple[float, float, float], end: tuple[float, float, float], radius: float, material: str, segments: int=20, export: bool=True) -> None:
        p0 = np.array(start, dtype=float)
        p1 = np.array(end, dtype=float)
        axis = p1 - p0
        length = np.linalg.norm(axis)
        if length <= 1e-06:
            raise ValueError(f'Cylinder {name} has zero length')
        axis /= length
        helper = np.array([0.0, 0.0, 1.0])
        if abs(float(np.dot(axis, helper))) > 0.92:
            helper = np.array([0.0, 1.0, 0.0])
        tangent = np.cross(axis, helper)
        tangent /= np.linalg.norm(tangent)
        bitangent = np.cross(axis, tangent)
        vertices: list[np.ndarray] = []
        for point in (p0, p1):
            for index in range(segments):
                angle = 2.0 * math.pi * index / segments
                offset = radius * (math.cos(angle) * tangent + math.sin(angle) * bitangent)
                vertices.append(point + offset)
        vertices.extend([p0, p1])
        faces: list[tuple[int, ...]] = []
        for index in range(segments):
            next_index = (index + 1) % segments
            faces.append((index, next_index, segments + next_index, segments + index))
        faces.append(tuple(range(segments - 1, -1, -1)))
        faces.append(tuple(range(segments, segments * 2)))
        self.objects.append(MeshObject(name, material, vertices, faces, export))

    def add_z_cylinder(self, name: str, center: tuple[float, float, float], radius: float, height: float, material: str, segments: int=32, export: bool=True) -> None:
        cx, cy, cz = center
        self.add_cylinder_between(name, (cx, cy, cz - height / 2.0), (cx, cy, cz + height / 2.0), radius, material, segments, export)

    def add_torus_z(self, name: str, center: tuple[float, float, float], major_radius: float, minor_radius: float, material: str, major_segments: int=32, minor_segments: int=10, export: bool=True) -> None:
        center_v = np.array(center, dtype=float)
        vertices: list[np.ndarray] = []
        for major_index in range(major_segments):
            u = 2.0 * math.pi * major_index / major_segments
            radial = np.array([math.cos(u), math.sin(u), 0.0])
            for minor_index in range(minor_segments):
                v = 2.0 * math.pi * minor_index / minor_segments
                point = center_v + (major_radius + minor_radius * math.cos(v)) * radial
                point += np.array([0.0, 0.0, minor_radius * math.sin(v)])
                vertices.append(point)
        faces: list[tuple[int, ...]] = []
        for major_index in range(major_segments):
            next_major = (major_index + 1) % major_segments
            for minor_index in range(minor_segments):
                next_minor = (minor_index + 1) % minor_segments
                a = major_index * minor_segments + minor_index
                b = next_major * minor_segments + minor_index
                c = next_major * minor_segments + next_minor
                d = major_index * minor_segments + next_minor
                faces.append((a, b, c, d))
        self.objects.append(MeshObject(name, material, vertices, faces, export))

def triangulate(face: tuple[int, ...]) -> list[tuple[int, int, int]]:
    return [(face[0], face[index], face[index + 1]) for index in range(1, len(face) - 1)]

MATERIALS.update({'RailBlue': (0.1, 0.16, 0.19), 'ServicePanel': (0.48, 0.5, 0.48), 'CopperCoil': (0.63, 0.25, 0.07), 'MachinedSteel': (0.43, 0.47, 0.48), 'PowerBlue': (0.02, 0.58, 0.84), 'SafetyOrange': (0.82, 0.25, 0.035)})
