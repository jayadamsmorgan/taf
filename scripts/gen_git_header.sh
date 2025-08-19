#!/usr/bin/env sh
# Generates a header with release metadata.
# - TAF_IS_RELEASE: 1 if HEAD has a tag beginning with "v", else 0
# - TAF_GIT_VERSION: git describe
# - TAF_GIT_BRANCH: current branch

# Don't crash if git isn't available; still emit a header.
set -u

# Detect if HEAD is tagged with something like v1.2.3
tag="$(git tag --points-at HEAD --list 'v*' 2>/dev/null | head -n1 || true)"

is_release=0
ver="$(git describe --always --dirty=+ --abbrev=8 2>/dev/null || echo "")"
# short branch name; empty in detached HEAD
branch="$(git symbolic-ref --short -q HEAD 2>/dev/null || echo "")"

if [ -n "${tag:-}" ]; then
  is_release=1
fi

printf '#pragma once\n'
printf '#define TAF_IS_RELEASE %s\n' "$is_release"
printf '#define TAF_GIT_VERSION "%s"\n' "$ver"
printf '#define TAF_GIT_BRANCH "%s"\n' "$branch"
