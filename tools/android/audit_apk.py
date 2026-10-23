#!/usr/bin/env python3
"""Check distributable contents; apksigner performs signature verification separately."""
import argparse
import hashlib
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("apk")
args = p.parse_args()
with zipfile.ZipFile(args.apk) as archive:
    names = archive.namelist()
    for name in names:
        lower = name.lower()
        assert not lower.endswith((".gba", ".agb", ".jks", ".keystore", ".pending")), name
        assert not lower.endswith("/gba_bios.bin"), name
        assert "roms/" not in lower, name
    firmware = archive.read("assets/payload/bios/open_gba_bios.firmware")
    assert len(firmware) == 16384
    assert hashlib.sha1(firmware).hexdigest() == "2e41f9d30ecdf1c4540edf89ac16f49bf327edb7"
    assert "assets/payload/bios/LICENSE.txt" in names
    assert "lib/arm64-v8a/libmain.so" in names
    assert "lib/arm64-v8a/libSDL2.so" in names
    assert all("/armeabi-v7a/" not in n and "/x86/" not in n for n in names)
print("APK contents verified: ARM64, open firmware licensed, no game ROM or signing keys")
