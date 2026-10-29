#!/usr/bin/env python3
"""Import address/boundary facts only from biosp4rk/mf-zm-info (USA).

No decompiled game implementation or cartridge bytes enter the repository.
Usage: python tools/import_fusion_symbols.py --reference <mf-zm-info checkout>
"""
import argparse
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
ROM_SHA = "ca33f4348c2c05dd330d37b97e2c5a69531dfe87"

def region(value):
    return value.get("U") if isinstance(value, dict) else value

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=pathlib.Path, required=True)
    args = parser.parse_args()
    source = args.reference.resolve()
    code = json.loads((source / "json/mf/code.json").read_text())
    entries = sorted((region(x["addr"]), region(x["size"]), x["mode"], x["name"])
                     for x in code if region(x["addr"]) is not None)
    for addr, size, mode, name in entries:
        if not (0xC0 <= addr < addr + size <= 0x800000 and mode in {"arm", "thumb"}):
            raise ValueError(f"Invalid USA boundary: {name}")
    symbols = ROOT / "symbols"
    symbols.mkdir(exist_ok=True)
    tsv = ["# addr\tmode\tname; USA facts from biosp4rk/mf-zm-info"]
    tsv += [f"0x{0x08000000+a:08X}\t{mode}\t{name}" for a, _, mode, name in entries]
    (symbols / "fusion_functions.tsv").write_text("\n".join(tsv)+"\n")
    data = ["# addr\tregion\tsize\tname; USA metadata only"]
    for filename in ("ram.json", "data.json"):
        for item in json.loads((source / "json/mf" / filename).read_text()):
            addr = region(item["addr"])
            if addr is None:
                continue
            if filename == "data.json":
                addr += 0x08000000
            segment = "rom" if addr >= 0x08000000 else "iwram" if addr >= 0x03000000 else "ewram"
            data.append(f"0x{addr:08X}\t{segment}\t0\t{item['name']}")
    (symbols / "fusion_data.tsv").write_text("\n".join(data)+"\n")
    toml = ["# Generated address/boundary metadata; never generated guest code.",
            "[program]", 'name = "Metroid Fusion (USA)"', 'id = "fusion_usa"',
            "load_address = 0x08000000", "size = 0x00800000", "entry_pc = 0x08000000",
            "codegen_shards = 16", "static_resume_all = true", "",
            "[identity]", f'sha1 = "{ROM_SHA}"', "",
            "[[extra_func]]", "addr = 0x08000000", 'mode = "arm"', 'name = "rom_header_start_vector"']
    # Gaps between verified executable extents are data, including assets after
    # the final code section. Embedded code is declared separately when observed.
    end = 4
    for addr, size, _, _ in entries:
        if addr > end:
            toml += ["", "[[data_range]]", f"start = 0x{0x08000000+end:08X}",
                     f"end = 0x{0x08000000+addr:08X}", 'note = "Non-code gap in USA function map"']
        end = max(end, addr + size)
    toml += ["", "[[data_range]]", f"start = 0x{0x08000000+end:08X}", "end = 0x08800000",
             'note = "USA cartridge assets and padding, beyond executable section"']
    (symbols / "fusion_usa.toml").write_text("\n".join(toml)+"\n")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    digest = hashlib.sha256((source / "json/mf/code.json").read_bytes()).hexdigest()
    (symbols / "fusion_reference.txt").write_text(
        f"https://github.com/biosp4rk/mf-zm-info\ncommit {revision}\ncode.json SHA256 {digest}\n")
    print(f"Imported {len(entries)} USA function entries; no ROM bytes or game C sources.")

if __name__ == "__main__":
    main()
