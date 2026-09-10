"""Compile all F/G/H files and report exactness, branches and address identities."""
import argparse
from collections import Counter
import hashlib
import json
import tempfile

from common import *


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assigned = targets()
    names = {t["name"] for t in assigned}
    if not assigned or len(names) != len(assigned):
        raise ValueError("Missing or duplicate assigned targets in the scope briefs")
    data, sections = match.load_exe()
    reference = relocs.reference_addresses(ROOT)
    results = []
    with tempfile.TemporaryDirectory(prefix="fgh_check_") as tmp:
        for file in sorted({t["file"] for t in assigned}):
            source, obj = ROOT / "LEGOLAND" / file, Path(tmp) / (file + ".obj")
            compile_source(source, obj)
            coff = relocs.Coff(obj)
            local = relocs.source_addresses(source)
            for name, address, wip in audit.annotated(source):
                address = int(address, 16)
                original, compiled, escapes = bodies(obj, name, address, data, sections)
                score, count, ok, reasons = match.compare(
                    original, compiled, len(original), sum(x.size for x in original), escapes
                )
                row = dict(file=file, name=name, address=hex(address), assigned=name in names,
                           wip=wip, normalized_exact=ok, mismatch=count-score,
                           instructions=len(compiled), original_instructions=len(original),
                           bytes=sum(x.size for x in compiled),
                           original_bytes=sum(x.size for x in original), reasons=reasons)
                if ok:
                    row["branches"] = branch_issues(coff, name, original, compiled, data, sections)
                    identities = relocs.inspect_function(coff, name, address, data, sections, local, reference)
                    row["relocations"] = identities
                    row["literals"] = literal_checks(coff, original, identities, data, sections)
                    row["exact_gate"] = (not row["branches"] and identities["status"] == "checked"
                                         and not identities["counts"].get("mismatches", 0)
                                         and not row["literals"]["issues"])
                else:
                    row["exact_gate"] = False
                results.append(row)
            print(file + ": compiled without warnings", flush=True)
    found = {r["name"] for r in results if r["assigned"]}
    if found != names:
        raise ValueError("Assigned target markers missing: " + ", ".join(sorted(names - found)))
    summary = Counter()
    for row in results:
        summary["functions"] += 1
        summary["exact_markers"] += not row["wip"]
        summary["normalized_exact"] += row["normalized_exact"]
        summary["assigned_exact_gate"] += row["assigned"] and row["exact_gate"]
        summary["assigned_open"] += row["assigned"] and not row["exact_gate"]
        summary["existing_normalized_regressions"] += not row["wip"] and not row["normalized_exact"]
    report = dict(summary=dict(summary), original_sha256=hashlib.sha256(data).hexdigest(),
                  flags=["/W3", "/O2", "/Gy", "/Gd"], functions=results)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report["summary"], indent=2))
    # Still-open assigned WIPs are expected; falsely exact markers fail.
    return int(any(not r["wip"] and not r["normalized_exact"] for r in results)
               or any(r["assigned"] and not r["wip"] and not r["exact_gate"] for r in results))


if __name__ == "__main__":
    raise SystemExit(main())
