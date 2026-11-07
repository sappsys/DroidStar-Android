#!/bin/sh
# Windows builds are produced on a Windows host. See windows/README.md and
# scripts/build-windows.ps1. This stub exists so the packaging layout matches Android.
echo "Build Windows on a Windows machine:" >&2
echo "  powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1" >&2
echo "Details: windows/README.md" >&2
exit 1
