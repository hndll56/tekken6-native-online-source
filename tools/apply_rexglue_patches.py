#!/usr/bin/env python3
"""Apply the project-owned ReXGlue SDK changes (patches/rexglue) to a fresh SDK checkout."""

from __future__ import annotations

import argparse
import os
import tempfile
import subprocess
import sys
from pathlib import Path

# (patch, directory inside the SDK it applies to, the upstream revision that directory must be at)
PATCHES = (
    ("0001-tekken6-sdk-changes.patch", ".", "f5337cdc947ff6d4c4196737e2c807a48f2a1fc2"),
    ("0002-ffmpeg-mingw.patch", "thirdparty/FFmpeg", "0604b464c7cb4ebc94940cf1f324a3b26b87717c"),
)


def git(repo: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(["git", "-C", str(repo), *args], check=check, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def apply(repo: Path, patch: Path, base: str) -> bool:
    """True when the patch is applied afterwards (now or already); False (with a message) when the checkout differs."""
    if not patch.is_file():
        print(f"Missing project patch: {patch}", file=sys.stderr)
        return False
    if git(repo, "apply", "--reverse", "--check", str(patch), check=False).returncode == 0:
        print(f"{patch.name}: already applied")
        return True
    revision = git(repo, "rev-parse", "HEAD").stdout.strip()
    if revision != base:
        print(f"{patch.name}: expected {repo} at {base}, found {revision}; refusing to patch a different revision.", file=sys.stderr)
        return False
    if git(repo, "status", "--porcelain", "--untracked-files=no").stdout.strip():
        print(f"{patch.name}: {repo} has local changes; refusing to overwrite them.", file=sys.stderr)
        return False
    git(repo, "apply", "--check", str(patch))
    git(repo, "apply", str(patch))
    print(f"{patch.name}: applied")
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdk", type=Path, default=Path(__file__).resolve().parents[1] / "third_party" / "rexglue-sdk",
                        help="ReXGlue SDK checkout (default: this repo's submodule)")
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    patch_dir = Path(__file__).resolve().parents[1] / "patches" / "rexglue"
    if not (sdk / ".git").exists():
        print(f"ReXGlue SDK checkout not found: {sdk}", file=sys.stderr)
        return 2
    # The Linux patch is stacked on the original title patch. Compare the
    # worktree against each known prefix using an isolated Git index so repeat
    # invocations are safe even though the original patch's context changed.
    series = [patch_dir / "0001-tekken6-sdk-changes.patch", patch_dir / "0003-linux-host.patch"]
    base = PATCHES[0][2]
    if git(sdk, "rev-parse", "HEAD").stdout.strip() != base:
        print(f"Expected SDK revision {base}; refusing a different revision", file=sys.stderr)
        return 2
    with tempfile.TemporaryDirectory(prefix="t6-patch-") as temp:
        env = {**os.environ, "GIT_INDEX_FILE": str(Path(temp) / "index")}
        def indexed(*arguments: str):
            return subprocess.run(["git", "-C", str(sdk), *arguments], env=env, text=True, capture_output=True)
        result = indexed("read-tree", "HEAD")
        if result.returncode:
            print(result.stderr, file=sys.stderr)
            return 2
        matching_prefix = None
        for count in range(len(series) + 1):
            if indexed("diff", "--quiet", "--ignore-submodules=dirty", "--").returncode == 0:
                matching_prefix = count
            if count < len(series):
                patch = series[count]
                if not patch.is_file():
                    print(f"Missing patch: {patch}", file=sys.stderr)
                    return 2
                result = indexed("apply", "--cached", str(patch))
                if result.returncode:
                    print(result.stderr, file=sys.stderr)
                    return 2
        if matching_prefix is None:
            print("SDK has changes outside the known patch series; refusing to overwrite them", file=sys.stderr)
            return 2
        for patch in series[matching_prefix:]:
            # git apply checks every hunk before writing and rejects conflicts.
            git(sdk, "apply", "--check", str(patch))
            git(sdk, "apply", str(patch))
            print(f"{patch.name}: applied")
        if matching_prefix == len(series):
            print("SDK patch series: already applied")
    name, sub, base = PATCHES[1]
    if not apply((sdk / sub).resolve(), patch_dir / name, base):
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
