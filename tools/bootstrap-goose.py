#!/usr/bin/env python3
"""Install the checksum-pinned official Goose v2026.3 host compiler."""

import argparse
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile

VERSION = "v2026.3"
RUNTIME = "goose runtime 4d3da949e563"
RELEASES = {
    "Linux": (
        "goose-v2026.3-linux-x64.tar.gz",
        "06ac30284338269d3996d457412bf4f2b47c1f9a5601de65b6317187fe78d5c9",
        "goose-v2026.3-linux-x64/goose",
    ),
    "Windows": (
        "goose-v2026.3-windows-x64.zip",
        "e30cfa398c0362cef4b8f311cf5496743920b6d3a6c4a6d818fc47ee1e56be7d",
        "goose-v2026.3-windows-x64/goose.exe",
    ),
}


def digest(path):
    checksum = hashlib.sha256()
    with path.open("rb") as archive:
        for chunk in iter(lambda: archive.read(1024 * 1024), b""):
            checksum.update(chunk)
    return checksum.hexdigest()


def safe_destination(root, name):
    destination = (root / name).resolve()
    if destination != root and root not in destination.parents:
        raise ValueError("Goose archive contains an unsafe path")
    return destination


def extract_archive(archive, destination):
    # Extract only regular files/directories, with paths confined to our new
    # temporary directory. Do not follow archive links or extract devices.
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive) as package:
            for item in package.infolist():
                target = safe_destination(destination, item.filename)
                if (item.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError("Goose archive contains a symbolic link")
                if item.is_dir():
                    target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with package.open(item) as source, target.open("wb") as output:
                        shutil.copyfileobj(source, output)
    else:
        with tarfile.open(archive, "r:gz") as package:
            for item in package:
                target = safe_destination(destination, item.name)
                if item.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                elif item.isfile():
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with package.extractfile(item) as source, target.open("wb") as output:
                        shutil.copyfileobj(source, output)
                    target.chmod(item.mode & 0o777)
                else:
                    raise ValueError("Goose archive contains a non-regular entry")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--destination", type=Path,
        default=Path(__file__).resolve().parent.parent / "work/goose",
        help="download cache and installation directory (default: work/goose)",
    )
    args = parser.parse_args()
    host = platform.system()
    if host not in RELEASES or platform.machine().lower() not in ("amd64", "x86_64"):
        parser.error("The pinned Goose release supports Linux x64 and Windows x64 hosts only")
    filename, expected_hash, executable = RELEASES[host]
    destination = args.destination.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    archive = destination / filename
    if not archive.is_file() or digest(archive) != expected_hash:
        url = f"https://github.com/aardappel/goose/releases/download/{VERSION}/{filename}"
        print(f"Downloading Goose {VERSION} for {host} x64", file=sys.stderr)
        temporary = archive.with_suffix(archive.suffix + ".download")
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "goose-os-bootstrap"})
            with urllib.request.urlopen(request, timeout=120) as response, temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
            if digest(temporary) != expected_hash:
                raise ValueError("Goose release SHA256 mismatch; refusing to install")
            os.replace(temporary, archive)
        finally:
            temporary.unlink(missing_ok=True)
    # Re-extract the verified archive even when it is cached, so compiler and
    # bundled standard library always come from the same pinned release.
    with tempfile.TemporaryDirectory(prefix=".extract-", dir=destination) as temporary:
        extraction_root = Path(temporary).resolve()
        extract_archive(archive, extraction_root)
        compiler = extraction_root / executable
        version = subprocess.run([str(compiler), "version"], check=True, capture_output=True, text=True)
        if version.stdout.strip() != RUNTIME:
            raise ValueError("Goose release reported an unexpected runtime version")
        package_name = Path(executable).parts[0]
        installed = destination / package_name
        if installed.exists():
            shutil.rmtree(installed)
        shutil.move(str(extraction_root / package_name), installed)
    print(destination / executable)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError, tarfile.TarError, zipfile.BadZipFile) as error:
        print(f"Goose installation failed: {error}", file=sys.stderr)
        raise SystemExit(1)
