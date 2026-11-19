"""Revision snapshots must preserve source bytes and reject escaping archive paths."""
import io
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

from blueprint_workspace import OWNER, write
from asset_text_codec import compact
from test_asset_text_codec import fixture
import git_review


def archive(name, value):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, 'w') as output:
        output.writestr(name, value)
    return stream.getvalue()


class GitReviewTests(unittest.TestCase):
    def test_review_distinguishes_unchanged_changed_added_removed(self):
        with tempfile.TemporaryDirectory(dir=OWNER) as directory:
            root = Path(directory)
            old, new = root / 'before.json', root / 'after.json'
            doc = compact(fixture())
            write(old, doc); write(new, doc)
            exports = dict(before={'same': old, 'removed': old}, after={'same': new, 'added': new})
            records = git_review.blueprint_records(['same', 'added', 'removed'], exports)
            self.assertEqual([r['status'] for r in records], ['unchanged', 'added', 'removed'])
            self.assertNotIn('before', records[0])
            doc['defaults']['Amount'] = '2.500000'
            write(new, doc)
            changed = git_review.blueprint_records(['same'], exports)[0]
            self.assertEqual(changed, dict(before=str(old), source=str(new), status='changed'))

    def test_extract_is_exact_and_rejects_escape_or_inconsistent_overlay(self):
        with tempfile.TemporaryDirectory(dir=OWNER) as directory:
            root = Path(directory)
            with patch.object(git_review, 'git', return_value=archive('Project/Content/Parser.uasset', b'asset')) as git:
                git_review.extract('revision', 'Project', root)
                git.assert_called_once_with('archive', '--format=zip', 'revision', '--', 'Project')
            self.assertEqual((root / 'Project/Content/Parser.uasset').read_bytes(), b'asset')
            with patch.object(git_review, 'git', return_value=archive('../escape.uasset', b'bad')):
                with self.assertRaisesRegex(ValueError, 'escapes'):
                    git_review.extract('revision', 'Project', root)
            with patch.object(git_review, 'git', return_value=archive('Project/Content/Parser.uasset', b'different')):
                with self.assertRaisesRegex(ValueError, 'collision'):
                    git_review.extract('revision', 'Project', root)
            self.assertEqual((root / 'Project/Content/Parser.uasset').read_bytes(), b'asset')


if __name__ == '__main__':
    unittest.main()
