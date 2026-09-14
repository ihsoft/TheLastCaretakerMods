"""Inspect the exported OBJ with backface culling and perspective-correct depth."""
import argparse
import math
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import geometry as core
from build_model import STEM, ROOT, read_obj

def unit(v):
    v = np.asarray(v, dtype=float)
    return v / np.linalg.norm(v)

def triangles(scene):
    for obj in scene.objects:
        for face in obj.faces:
            for t in core.triangulate(face):
                p = np.array([obj.vertices[i] for i in t])
                yield (p, unit(np.cross(p[1] - p[0], p[2] - p[0])), obj.material)

def pixels(p, width, height):
    x0, y0 = np.maximum(np.floor(p[:, :2].min(axis=0)).astype(int), 0)
    x1, y1 = np.minimum(np.ceil(p[:, :2].max(axis=0)).astype(int), [width - 1, height - 1])
    if x1 < x0 or y1 < y0:
        return None
    ax, ay = p[0, :2]
    bx, by = p[1, :2]
    cx, cy = p[2, :2]
    denom = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
    if abs(denom) < 1e-09:
        return None
    xx, yy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
    a = ((by - cy) * (xx - cx) + (cx - bx) * (yy - cy)) / denom
    b = ((cy - ay) * (xx - cx) + (ax - cx) * (yy - cy)) / denom
    c = 1 - a - b
    mask = (a >= -1e-09) & (b >= -1e-09) & (c >= -1e-09)
    return (x0, y0, x1, y1, np.stack([a, b, c], axis=-1), mask)

class Shadow:

    def __init__(self, tris, key):
        self.size = 1536
        self.right = unit(np.cross(-key, [0, 0, 1]))
        self.up = unit(np.cross(self.right, -key))
        self.basis = np.array([self.right, self.up, -key]).T
        all_p = np.concatenate([t[0] for t in tris]) @ self.basis
        self.low = all_p.min(axis=0) - 8
        span = all_p.max(axis=0) - self.low + 8
        self.scale = (self.size - 1) / max(span[:2])
        self.depth = np.full((self.size, self.size), np.inf, dtype=np.float32)
        for points, _, _ in tris:
            p = self.project(points)
            frag = pixels(p, self.size, self.size)
            if frag is None:
                continue
            x0, y0, x1, y1, bary, inside = frag
            z = bary @ p[:, 2]
            part = self.depth[y0:y1 + 1, x0:x1 + 1]
            np.minimum(part, np.where(inside, z, np.inf), out=part)

    def project(self, p):
        out = np.asarray(p) @ self.basis - self.low
        out[..., :2] *= self.scale
        return out

    def visibility(self, world, normal, key):
        p = self.project(world)
        xy = np.rint(p[..., :2]).astype(int)
        vis = np.zeros(p.shape[:-1], dtype=np.float32)
        bias = 0.2 + 0.3 * (1 - max(0, float(np.dot(normal, key))))
        for dx, dy in [(0, 0), (-2, -2), (-2, 2), (2, -2), (2, 2)]:
            x = np.clip(xy[..., 0] + dx, 0, self.size - 1)
            y = np.clip(xy[..., 1] + dy, 0, self.size - 1)
            vis += (p[..., 2] - bias <= self.depth[y, x]) / 5
        return vis

def render(scene, size, camera, target, ortho=False, shadow=True, reverse=False):
    w, h = size
    camera = np.array(camera, dtype=float)
    forward = unit(np.array(target) - camera)
    right = unit(np.cross(forward, [0, 0, 1]))
    up = unit(np.cross(right, forward))
    basis = np.array([right, up, forward]).T
    focal = w / (2 * math.tan(math.radians(33) / 2))
    scale = w / 375
    image = np.empty((h, w, 3), dtype=np.float32)
    for y in range(h):
        image[y] = np.array([0.14, 0.18, 0.21]) * (1 - 0.35 * y / h)
    depth = np.full((h, w), np.inf, dtype=np.float32)
    tris = list(triangles(scene))
    key = unit([-0.4, -0.7, 1.1])
    sm = Shadow(tris, key) if shadow else None
    fill = unit([0.8, 0.2, 0.6])
    if shadow:
        floor = np.array([[-1200, -1200, -0.15], [1200, -1200, -0.15], [1200, 1200, -0.15], [-1200, 1200, -0.15]])
        floor[:, :2] *= 0.25
        tris.extend([(floor[[0, 1, 2]], np.array([0.0, 0.0, 1.0]), 'PreviewDeck'), (floor[[0, 2, 3]], np.array([0.0, 0.0, 1.0]), 'PreviewDeck')])
    if reverse:
        tris.reverse()
    for points, normal, material in tris:
        if np.dot(normal, camera - points.mean(axis=0) if not ortho else -forward) <= 0:
            continue
        cp = (points - camera) @ basis
        if (cp[:, 2] <= 1).any():
            continue
        p = cp.copy()
        if ortho:
            p[:, 0] = w / 2 + cp[:, 0] * scale
            p[:, 1] = h / 2 - cp[:, 1] * scale
        else:
            p[:, 0] = w / 2 + cp[:, 0] * focal / cp[:, 2]
            p[:, 1] = h / 2 - cp[:, 1] * focal / cp[:, 2]
        frag = pixels(p, w, h)
        if frag is None:
            continue
        x0, y0, x1, y1, bary, inside = frag
        if ortho:
            z = bary @ cp[:, 2]
            weights = bary
        else:
            invz = bary @ (1 / cp[:, 2])
            z = 1 / np.maximum(invz, 1e-10)
            weights = bary / cp[:, 2] * z[..., None]
        current = depth[y0:y1 + 1, x0:x1 + 1]
        keep = inside & (z < current - 1e-05)
        if not keep.any():
            continue
        world = weights[keep] @ points
        visibility = sm.visibility(world, normal, key) if sm else np.ones(len(world))
        kd = max(0, float(normal @ key))
        fd = max(0, float(normal @ fill))
        light = (0.44 + 0.82 * kd * visibility + 0.3 * fd)[:, None]
        color = np.array(core.MATERIALS[material])
        linear = color ** 2.2 * light
        view = camera - world
        view /= np.linalg.norm(view, axis=1)[:, None]
        half = view + key
        half /= np.linalg.norm(half, axis=1)[:, None]
        spec = np.maximum(0, half @ normal) ** 45 * visibility
        spec_amount = 0.16 if material in ['MachinedSteel', 'EdgeMetal', 'CopperCoil'] else 0.035
        linear += spec[:, None] * spec_amount
        if material == 'PowerBlue':
            linear += color ** 2.2 * 0.4
        if material == 'PreviewDeck':
            contact = np.exp(-((world[:, 0] / 95) ** 2 + (world[:, 1] / 90) ** 2)) * 0.35
            linear *= (1 - contact)[:, None]
        rgb = np.clip(linear, 0, 1) ** (1 / 2.2)
        image[y0:y1 + 1, x0:x1 + 1][keep] = rgb
        current[keep] = z[keep]
    return Image.fromarray((np.clip(image, 0, 1) * 255).astype(np.uint8))

def _render_all():
    scene = read_obj(ROOT / (STEM + '.obj'))
    hero = render(scene, (2400, 1600), (470, -570, 325), (23, -8, 82))
    hero.save(ROOT / (STEM + '_hero.png'))
    print('Hero rendered from exported OBJ', flush=True)
    sheet = Image.new('RGB', (1920, 1280), (22, 29, 35))
    views = [('LEFT / 5 MAGNETS', (30, -650, 105), (30, 0, 105), True), ('RIGHT / 5 MAGNETS', (30, 650, 105), (30, 0, 105), True), ('REAR / CLOSED CAPACITORS', (-480, 420, 290), (5, 0, 80), False), ('FRONT / FRAME AND BORE', (580, -270, 230), (30, 0, 83), False)]
    for i, (label, cam, target, ortho) in enumerate(views):
        img = render(scene, (960, 640), cam, target, ortho, shadow=False)
        draw = ImageDraw.Draw(img)
        font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 20)
        draw.text((20, 18), label, font=font, fill=(230, 235, 240))
        sheet.paste(img, (i % 2 * 960, i // 2 * 640))
        print(label, flush=True)
    sheet.save(ROOT / (STEM + '_inspection.png'))
    evidence = {}
    tests = [('hero', (470, -570, 325), (23, -8, 82), False)] + views
    for label, cam, target, ortho in tests:
        a = np.array(render(scene, (480, 320), cam, target, ortho, shadow=False))
        b = np.array(render(scene, (480, 320), cam, target, ortho, shadow=False, reverse=True))
        diff = np.max(np.abs(a.astype(int) - b.astype(int)), axis=2)
        evidence[label] = int((diff > 1).sum())
        assert evidence[label] == 0, ('Possible coplanar overlap or raster depth fault', label)
    (ROOT / (STEM + '_render_audit.json')).write_text(json.dumps({'source': 'Exported OBJ readback', 'imageGenerationUsed': False, 'depth': 'Perspective-correct per-pixel z-buffer', 'backfaceCulling': True, 'orderTestSize': [480, 320], 'orderTestChangedPixels': evidence, 'heroSize': [2400, 1600], 'inspectionSize': [1920, 1280], 'limitations': 'Source renderer only; not UE/game validation. Flat face normals; final UVs/materials pending.'}, indent=2), encoding='utf-8')
    print('Reversed triangle order:', evidence, flush=True)

def _render_details():
    scene = read_obj(ROOT / (STEM + '.obj'))

    detail = Image.new('RGB', (1800, 1000), (22, 29, 35))

    views = [('CENTERED FIXED POWER INLET', (75, -245, 85), (0, -62, 22)), ('CHAMFERED MUZZLE / 10 CM BORE', (340, -128, 190), (177, 0, 111))]

    for i, (label, cam, target) in enumerate(views):
        img = render(scene, (900, 1000), cam, target, shadow=False)
        ImageDraw.Draw(img).text((20, 20), label, font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 19), fill=(230, 235, 240))
        detail.paste(img, (900 * i, 0))

    detail.save(ROOT / (STEM + '_details.png'))


def main():
    global ROOT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT,
                        help='Directory containing generated OBJ; receives renders and audit')
    args=parser.parse_args()
    ROOT=args.output.resolve()
    source=Path(__file__).resolve().parent
    if ROOT==source or source in ROOT.parents:
        parser.error('Render output must stay outside accepted source directory')
    _render_all()
    _render_details()


if __name__=='__main__': main()
