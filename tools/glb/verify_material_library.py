"""Read-only integrity check for Export-VoyageMaterialsGlb output, including every PNG."""
import argparse
import hashlib
import io
import json
import struct
from pathlib import Path

from PIL import Image


def verify(path):
    data = Path(path).read_bytes()
    if struct.unpack_from('<III', data) != (0x46546C67, 2, len(data)):
        raise ValueError('Invalid GLB header')
    length, kind = struct.unpack_from('<II', data, 12)
    if kind != 0x4E4F534A or length % 4:
        raise ValueError('Missing JSON chunk')
    doc = json.loads(data[20:20 + length])
    size, kind = struct.unpack_from('<II', data, 20 + length)
    binary = data[28 + length:]
    if kind != 0x004E4942 or size != len(binary):
        raise ValueError('Invalid embedded BIN')
    if len(doc['buffers']) != 1 or 'uri' in doc['buffers'][0]:
        raise ValueError('Not self-contained')
    evidence = doc['extras']
    schema = evidence['schemaVersion']
    if schema not in (1, 2) or len(evidence['materials']) != len(doc['materials']):
        raise ValueError('Material provenance/count mismatch')
    images = []
    for image in doc.get('images', []):
        if image.get('mimeType') != 'image/png' or 'uri' in image:
            raise ValueError('Expected embedded PNG')
        view = doc['bufferViews'][image['bufferView']]
        offset, count = view.get('byteOffset', 0), view['byteLength']
        if view.get('buffer', 0) != 0 or offset < 0 or count <= 0 or offset + count > len(binary):
            raise ValueError('Image out of bounds')
        png = binary[offset:offset + count]
        with Image.open(io.BytesIO(png)) as decoded:
            dimensions = decoded.size
            decoded.verify()
        images.append((hashlib.sha256(png).hexdigest().upper(), dimensions))
    failures = 0
    described = set()
    for record in evidence['textures']:
        if record['Error'] is not None:
            failures += 1
            if (schema == 1 and record['ImageIndex'] is not None) or (schema == 2 and record['Variants']):
                raise ValueError('Failed texture incorrectly claims an image')
            continue
        variants = [record] if schema == 1 else record['Variants']
        if not variants:
            raise ValueError('Decoded resource without a used variant')
        for variant in variants:
            index = variant['ImageIndex']
            if not isinstance(index, int) or not 0 <= index < len(images):
                raise ValueError('Invalid provenance image index')
            digest, dimensions = images[index]
            if digest != variant['Sha256'] or dimensions != (record['Width'], record['Height']):
                raise ValueError('Texture provenance/hash/dimensions mismatch')
            described.add(index)
    for index, record in enumerate(evidence['materials']):
        material = doc['materials'][index]
        if material['extras']['Source'] != record['Source']:
            raise ValueError('Material identity mismatch')
        if not record['Warnings']:
            raise ValueError('Missing approximation disclosure')
    for texture in doc.get('textures', []):
        if not 0 <= texture['source'] < len(images):
            raise ValueError('Invalid texture/image link')
    if schema == 2:
        used_textures = set()
        for material in doc['materials']:
            for key in ('normalTexture', 'occlusionTexture', 'emissiveTexture'):
                if key in material:
                    used_textures.add(material[key]['index'])
            for key in ('baseColorTexture', 'metallicRoughnessTexture'):
                if key in material.get('pbrMetallicRoughness', {}):
                    used_textures.add(material['pbrMetallicRoughness'][key]['index'])
            if 'emissiveTexture' in material and not any(material.get('emissiveFactor', [0, 0, 0])):
                raise ValueError('Disabled emission unnecessarily carries a texture')
        textures = doc.get('textures', [])
        if used_textures != set(range(len(textures))):
            raise ValueError('Unbound texture or invalid material binding')
        used_images = {textures[i]['source'] for i in used_textures}
        if used_images != set(range(len(images))) or described != used_images:
            raise ValueError('Unbound or unprovenanced image payload')
    return {'status': 'verified', 'glb': str(Path(path).resolve()),
            'materials': len(doc['materials']), 'images': len(images),
            'unavailableTextures': failures, 'selfContained': True,
            'sha256': hashlib.sha256(data).hexdigest().upper()}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('glb')
    args = parser.parse_args()
    print(json.dumps(verify(args.glb)))
