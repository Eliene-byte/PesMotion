#!/usr/bin/env python3
"""PESMotion - validate_downloads: valida BVHs baixados rodando o engine.

Para cada *.bvh encontrado em --dir, executa `<bin> info <arquivo>` e registra
o resultado em --report. Erros são registrados, não fatais.
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="datasets_out")
    ap.add_argument("--bin", default="./build/pesmotion")
    ap.add_argument("--report", default=None)
    args = ap.parse_args()

    root = Path(args.dir)
    results = []
    for bvh in sorted(root.rglob("*.bvh")):
        try:
            proc = subprocess.run([args.bin, "info", str(bvh)],
                                  capture_output=True, text=True, timeout=30)
            ok = proc.returncode == 0
            results.append({"file": str(bvh), "ok": ok,
                            "stdout": proc.stdout[:2000], "stderr": proc.stderr[:2000]})
        except Exception as exc:  # noqa: BLE001
            results.append({"file": str(bvh), "ok": False, "error": str(exc)})

    summary = {"bvh_files": len(results),
               "ok": sum(1 for r in results if r["ok"]),
               "failed": sum(1 for r in results if not r["ok"])}
    print(json.dumps(summary, indent=2, ensure_ascii=False))

    if args.report:
        Path(args.report).write_text(
            json.dumps({"summary": summary, "results": results}, indent=2, ensure_ascii=False),
            encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
