#!/usr/bin/env python3
"""PESMotion - download_datasets: busca datasets de mocap abertos no CI.

Regras de ouro:
  * Nada aqui é obrigatório para compilar o PesMotion.
  * Apenas fontes com "enabled": true no manifest são baixadas.
  * Erros de rede/HTTP nunca derrubam o build principal (continue-on-error).
  * Nada é commitado de volta: os dados ficam só no artifact do job.
  * Licenças são responsabilidade de quem habilita a fonte — verifique
    datasets/manifest.json antes de habilitar.
"""

import argparse
import hashlib
import json
import sys
import time
import urllib.request
from pathlib import Path

USER_AGENT = "PESMotion-CI/0.1 (+https://github.com/Eliene-byte/PesMotion)"


def fetch(url: str, dest: Path, max_bytes: int, timeout: int = 60) -> dict:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    result = {"url": url, "file": str(dest), "status": "error", "bytes": 0, "sha256": None, "error": None}
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            data = resp.read(max_bytes + 1)
        if len(data) > max_bytes:
            result["error"] = f"excedeu limite de {max_bytes} bytes"
            return result
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        result["status"] = "ok"
        result["bytes"] = len(data)
        result["sha256"] = hashlib.sha256(data).hexdigest()
    except Exception as exc:  # noqa: BLE001 - CI deve registrar, não derrubar
        result["error"] = str(exc)
    return result


def main() -> int:
    ap = argparse.ArgumentParser(description="Baixa datasets abertos declarados no manifest")
    ap.add_argument("--manifest", default="datasets/manifest.json")
    ap.add_argument("--out", default="datasets_out")
    ap.add_argument("--report", default=None)
    args = ap.parse_args()

    manifest_path = Path(args.manifest)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    report = {"manifest": str(manifest_path), "started_at": int(time.time()),
              "sources": [], "summary": {"enabled": 0, "ok": 0, "errors": 0, "skipped": 0}}

    if not manifest_path.exists():
        report["summary"]["errors"] = 1
        report["fatal"] = f"manifest não encontrado: {manifest_path}"
        _write_report(report, args.report)
        print(f"manifest não encontrado: {manifest_path}", file=sys.stderr)
        return 0  # nunca derruba o CI

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    policy = manifest.get("policy", {})
    max_bytes = int(policy.get("max_bytes_per_source", 2 * 1024 * 1024 * 1024))
    skip_on_error = bool(policy.get("skip_on_error", True))

    for src in manifest.get("sources", []):
        entry = {"id": src.get("id"), "name": src.get("name"), "enabled": bool(src.get("enabled", False))}
        if not src.get("enabled", False):
            entry["status"] = "skipped (disabled)"
            report["summary"]["skipped"] += 1
            report["sources"].append(entry)
            continue
        url = src.get("url")
        if not url:
            entry["status"] = "error"
            entry["error"] = "fonte habilitada sem 'url' direta"
            report["summary"]["errors"] += 1
            report["sources"].append(entry)
            if not skip_on_error:
                _write_report(report, args.report)
                return 1
            continue
        report["summary"]["enabled"] += 1
        dest = out_dir / (src.get("id") or "fonte") / Path(url.split("?")[0]).name
        r = fetch(url, dest, max_bytes)
        entry.update(r)
        if r["status"] == "ok":
            report["summary"]["ok"] += 1
        else:
            report["summary"]["errors"] += 1
        report["sources"].append(entry)

    report["finished_at"] = int(time.time())
    _write_report(report, args.report)
    print(json.dumps(report["summary"], indent=2, ensure_ascii=False))
    return 0


def _write_report(report: dict, report_path) -> None:
    if not report_path:
        return
    Path(report_path).write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")


if __name__ == "__main__":
    sys.exit(main())
