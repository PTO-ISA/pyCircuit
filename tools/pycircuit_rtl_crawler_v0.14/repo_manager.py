from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Dict


class RepoError(RuntimeError):
    pass


def _run(cmd, cwd=None, capture=False):
    kwargs = {
        "cwd": str(cwd) if cwd else None,
        "check": True,
        "text": True,
    }
    if capture:
        kwargs["stdout"] = subprocess.PIPE
        kwargs["stderr"] = subprocess.PIPE
    try:
        return subprocess.run(cmd, **kwargs)
    except subprocess.CalledProcessError as exc:
        raise RepoError(
            "Command failed: {}\n{}".format(
                " ".join(cmd),
                getattr(exc, "stderr", "") or ""
            )
        ) from exc


def _materialize_sparse_paths(repo_dir: Path, source: Dict) -> None:
    """Materialize source-declared paths in an existing sparse checkout.

    The crawler keeps large repositories sparse during discovery, but a
    candidate's dependency closure may legitimately reach a small set of
    legacy/vendor files outside the normal ``path_hints`` (CVA6's
    ``common/local`` SRAM wrappers are one example).  Expanding the sparse
    checkout here keeps the files in their original repository, preserving
    provenance instead of copying an untracked substitute into a candidate.
    """
    paths = [str(p).replace('\\\\', '/').strip('/')
             for p in (source.get('sparse_paths', []) or []) if str(p).strip()]
    if not paths:
        return
    try:
        enabled = _run(["git", "config", "--get", "core.sparseCheckout"],
                       cwd=repo_dir, capture=True).stdout.strip().lower()
    except RepoError:
        enabled = ""
    if enabled != "true":
        return
    # Newer Git has ``sparse-checkout add``.  The WSL image used by the
    # crawler currently ships Git 2.25, which predates that subcommand and
    # also refuses to rewrite patterns when the cache has line-ending
    # changes.  Fall back to checkout-index for only the declared paths; this
    # materializes tracked files without touching unrelated worktree files.
    try:
        _run(["git", "sparse-checkout", "add", "--skip-checks", *paths], cwd=repo_dir, capture=True)
        return
    except RepoError:
        pass
    for path in paths:
        listing = _run(["git", "ls-files", "--", path], cwd=repo_dir, capture=True).stdout
        for tracked in (line.strip() for line in listing.splitlines() if line.strip()):
            _run(["git", "checkout-index", "--force", "--", tracked], cwd=repo_dir)


def ensure_repo(source: Dict, repos_dir: Path, update: bool = True) -> Path:
    project = source["project"]
    url = source["repo"]
    repo_dir = repos_dir / project

    repos_dir.mkdir(parents=True, exist_ok=True)

    if not repo_dir.exists():
        cmd = ["git", "clone", "--depth", "1", "--no-tags"]
        branch = source.get("branch")
        if branch:
            cmd += ["--branch", branch]
        cmd += [url, str(repo_dir)]
        print("[clone] {} <- {}".format(project, url))
        _run(cmd)
    elif update:
        print("[update] {}".format(project))
        _run(["git", "fetch", "--depth", "1", "--no-tags", "origin"], cwd=repo_dir)
        _run(["git", "reset", "--hard", "FETCH_HEAD"], cwd=repo_dir)
    else:
        print("[reuse] {}".format(project))

    _materialize_sparse_paths(repo_dir, source)

    return repo_dir


def git_metadata(repo_dir: Path) -> Dict[str, str]:
    sha = _run(
        ["git", "rev-parse", "HEAD"], cwd=repo_dir, capture=True
    ).stdout.strip()
    remote = _run(
        ["git", "remote", "get-url", "origin"], cwd=repo_dir, capture=True
    ).stdout.strip()
    try:
        branch = _run(
            ["git", "branch", "--show-current"], cwd=repo_dir, capture=True
        ).stdout.strip()
    except RepoError:
        branch = ""
    return {
        "commit_sha": sha,
        "remote_url": remote,
        "branch": branch,
    }
