#!/usr/bin/env python3

import getpass
import re
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path

APP_FILE = "meta.xml"
ICON_FILE = "icon.png"


def find_dol():
    dol_files = sorted(Path(".").glob("*.dol"))
    if not dol_files:
        return None
    if len(dol_files) == 1:
        return dol_files[0]

    print("Multiple .dol files found:")
    for i, dol in enumerate(dol_files, start=1):
        print(f"  {i}) {dol.name}")

    while True:
        choice = input("Which one should be used? [1]: ").strip()
        if not choice:
            return dol_files[0]
        try:
            index = int(choice)
            if 1 <= index <= len(dol_files):
                return dol_files[index - 1]
        except ValueError:
            pass


def detect_drives():
    drives = []
    root = Path("/run/media") / getpass.getuser()
    if not root.is_dir():
        return drives

    for mountpoint in sorted(root.iterdir()):
        if not mountpoint.is_dir() or mountpoint.name.startswith("."):
            continue
        if not (mountpoint / "apps").is_dir():
            continue
        drives.append(
            {
                "name": mountpoint.name,
                "mountpoint": str(mountpoint),
                "size": "",
            }
        )
    return drives


def choose_drive(drives):
    if not drives:
        print("Error: No external (homebrew) drives detected.")
        return None

    print("\nExternal drives detected:")
    for i, drive in enumerate(drives, start=1):
        print(
            f"  {i}) {drive['name']}  ({drive['size']})  {drive['mountpoint']}"
        )

    while True:
        choice = input(
            "Which one is the homebrew drive? [1]: "
        ).strip()
        if not choice:
            return drives[0]
        try:
            index = int(choice)
            if 1 <= index <= len(drives):
                return drives[index - 1]
        except ValueError:
            pass


def app_name_from_meta(meta_path):
    try:
        root = ET.parse(meta_path).getroot()
        raw = root.findtext("name", "").strip()
    except (ET.ParseError, FileNotFoundError):
        raw = ""
    name = re.sub(r"\s+", "-", raw).lower()
    name = re.sub(r"[^a-z0-9-]", "", name)
    return name or "app"


def main():
    print("=== Wii Homebrew Installer ===\n")

    dol = find_dol()
    if dol is None:
        print("Error: No .dol file found in the current directory.")
        return

    drive = choose_drive(detect_drives())
    if drive is None:
        return

    app_name = app_name_from_meta(APP_FILE)
    target_dir = Path(drive["mountpoint"]) / "apps" / app_name

    files = [
        (dol, "boot.dol"),
        (Path(APP_FILE), APP_FILE),
        (Path(ICON_FILE), ICON_FILE),
    ]

    target_dir.mkdir(parents=True, exist_ok=True)

    copied = []
    for source, arcname in files:
        if not source.is_file():
            print(f"  - {source.name} not found, skipping")
            continue
        shutil.copy2(source, target_dir / arcname)
        copied.append(arcname)

    app_path = target_dir / "boot.dol"
    try:
        app_path.chmod(0o755)
    except OSError:
        pass

    print(f"\nDone! Installed to: {target_dir}")
    for arcname in copied:
        print(f"  ✓ {arcname}")


if __name__ == "__main__":
    main()
