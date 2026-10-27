#!/usr/bin/env python3
"""Lays out a client release: Metin2.exe, data/<pack>.zip for every pack folder of the game
directory and lib/ (the embedded Python's standard library) as it is.

    package_release.py --exe build/bin/client.exe --game ../m2game --out release [--archive release.zip]

The client opens data/<name>.zip in place of data/<name>/, so each zip holds the folder's files with
paths relative to it. --archive also writes the distribution zip, with everything under Metin2/.
"""

import argparse
import os
import shutil
import sys
import zipfile

# Files left out of the packs, by extension (lower case): source assets the client never reads.
EXCLUDE_EXTENSIONS = {".ase"}


def pack_folder(folder, zip_path):
    count = 0
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as archive:
        for root, dirs, files in os.walk(folder):
            dirs.sort()
            for name in sorted(files):
                if os.path.splitext(name)[1].lower() in EXCLUDE_EXTENSIONS:
                    continue
                path = os.path.join(root, name)
                archive.write(path, os.path.relpath(path, folder).replace(os.sep, "/"))
                count += 1
    return count


def write_archive(out, archive_path):
    with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as archive:
        for root, dirs, files in os.walk(out):
            dirs.sort()
            for name in sorted(files):
                path = os.path.join(root, name)
                # The packs are deflated already.
                compression = zipfile.ZIP_STORED if name.lower().endswith(".zip") else zipfile.ZIP_DEFLATED
                arcname = "Metin2/" + os.path.relpath(path, out).replace(os.sep, "/")
                archive.write(path, arcname, compress_type=compression)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", required=True, help="the built client executable")
    parser.add_argument("--game", required=True, help="the game directory (an m2game checkout)")
    parser.add_argument("--out", required=True, help="the release folder, recreated empty")
    parser.add_argument("--archive", help="also write the distribution zip here")
    args = parser.parse_args()

    data = os.path.join(args.game, "data")
    lib = os.path.join(args.game, "lib")
    for path in (args.exe, data, lib):
        if not os.path.exists(path):
            sys.exit("not found: " + path)

    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    os.makedirs(os.path.join(args.out, "data"))

    shutil.copy2(args.exe, os.path.join(args.out, "Metin2.exe"))
    for name in sorted(os.listdir(data)):
        source = os.path.join(data, name)
        target = os.path.join(args.out, "data", name)
        if os.path.isdir(source):
            count = pack_folder(source, target + ".zip")
            print("data/%s.zip: %d files, %.1f MiB" % (name, count, os.path.getsize(target + ".zip") / 1048576.0))
        else:
            shutil.copy2(source, target)
            print("data/%s" % name)
    shutil.copytree(lib, os.path.join(args.out, "lib"))

    if args.archive:
        write_archive(args.out, args.archive)
        print("%s: %.1f MiB" % (args.archive, os.path.getsize(args.archive) / 1048576.0))


if __name__ == "__main__":
    main()
