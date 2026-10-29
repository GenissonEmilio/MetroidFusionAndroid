#!/usr/bin/env python3
"""Source-only CI guard: public sources must not include private game payloads."""
import hashlib
import pathlib
import subprocess
import tomllib

root = pathlib.Path(__file__).resolve().parents[2]
firmware = root / "android/firmware/open_gba_bios.firmware"
assert len(firmware.read_bytes()) == 16384
assert hashlib.sha1(firmware.read_bytes()).hexdigest() == "2e41f9d30ecdf1c4540edf89ac16f49bf327edb7"
assert tomllib.loads((root / "android/game_android.toml").read_text())["rom"]["sha1"] == "ca33f4348c2c05dd330d37b97e2c5a69531dfe87"
paths = subprocess.check_output(["git", "ls-files", "-z"], cwd=root).decode().split("\0")
for path in filter(None, paths):
    p = pathlib.PurePosixPath(path)
    assert p.suffix.lower() not in {".gba", ".agb", ".apk", ".aab", ".jks", ".keystore"}, path
    assert not (path.startswith("generated/") and p.suffix in {".cpp", ".h", ".c"}), path
    assert p.name not in {"signing.local.properties", "local.properties", "gba_bios.bin"}, path
print("Source payload and firmware checks passed")
