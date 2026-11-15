# Asset packaging helpers

Use the public PowerShell operations, not these internal workers:

- [DataAsset conversion](../data-asset-conversion.md): convert a fresh cooked
  native DataAsset under an explicit serialization contract and compare its data.
- [Additional-file PAK](../pak-files.md): package an explicit source/mount-path
  list and verify the produced archive.

For tool changes, `test_packaging.py` covers identity, mount-path and preservation
boundaries without Unreal. Native acceptance uses the public tools on disposable
inputs; changed conversion also needs a consuming build. Tests do not establish
gameplay compatibility.
