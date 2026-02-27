# Based on https://github.com/NixOS/nixpkgs/blob/0182a361324364ae3f436a63005877674cf45efb/pkgs/by-name/vp/vpkedit/package.nix#L31-L54
#
# 5.0.0.5
# |
# --> ext/shared/sourcepp @ f90f84b
#   |
#   --> bufferstream # a7a727dd679c87d1b22a57785cfec9908fcd8a04
#   |
#   --> compressonator # 4d7469c41629f19b7d889592cab8a4115931339c
#   |
#   --> cryptopp-cmake # 866aceb8b13b6427a3c4541288ff412ad54f11ea
#   | |
#   | --> cryptopp * master
#   |
#   --> hat-trie # f1380d704eccf753db5f9df175789fff8ff353e0
#   |
#   --> miniz # 174573d60290f447c13a2b1b3405de2b96e27d6c
#   |
#   --> minizip-ng (craftablescience fork) # 567affbf0caf11c5a0e1f76f722fbadc65c6efd9
#   | |
#   | --> zlib-ng * stable
#   | |
#   | --> bzip2 * master
#   | |
#   | --> liblzma * master
#   | |
#   | --> zstd * release
#   |
#   --> qoi # 44b233a95eda82fbd2e39a269199b73af0f4c4c3
#   |
#   --> webp # 934b7d7448c2d2850be9fa3aa4a924d51fff9823
#

import re
import subprocess
import yaml

table = [
    [
        "bufferstream",
        "https://github.com/craftablescience/BufferStream",
        "a7a727dd679c87d1b22a57785cfec9908fcd8a04",
    ],
    [
        "cmp_compressonator",
        "https://github.com/craftablescience/compressonator",
        "4d7469c41629f19b7d889592cab8a4115931339c",
    ],
    ["cryptopp-cmake", "https://github.com/abdes/cryptopp-cmake", "master"],
    ["cryptopp", "https://github.com/weidai11/cryptopp", "master", "CRYPTOPP_SOURCES"],
    [
        "tsl_hat_trie",
        "https://github.com/Tessil/hat-trie",
        "f1380d704eccf753db5f9df175789fff8ff353e0",
    ],
    [
        "miniz",
        "https://github.com/richgel999/miniz",
        "174573d60290f447c13a2b1b3405de2b96e27d6c",
    ],
    [
        "minizip-ng",
        "https://github.com/craftablescience/minizip-ng",
        "567affbf0caf11c5a0e1f76f722fbadc65c6efd9",
    ],
    ["zlib", "https://github.com/zlib-ng/zlib-ng", "stable"],
    ["bzip2", "https://gitlab.com/bzip2/bzip2.git", "master"],
    ["liblzma", "https://github.com/tukaani-project/xz", "master"],
    ["zstd", "https://github.com/facebook/zstd", "release"],
    [
        "qoi",
        "https://github.com/phoboslab/qoi",
        "44b233a95eda82fbd2e39a269199b73af0f4c4c3",
    ],
    [
        "webp",
        "https://github.com/webmproject/libwebp",
        "934b7d7448c2d2850be9fa3aa4a924d51fff9823",
    ],
]

SHA1_REGEX = r"^[0-9a-f]{40}$"

"""
Little script to help generate dependencies for offline builds of VPKEdit
Nix folks or anyone can modify to your needs
"""


def main():
    sources: list[dict[str, str]] = []
    build_options: list[str] = []
    deps_dir = "_deps"

    for dependency in table:
        name, url, commit, *custom_env = dependency
        if custom_env:
            custom_env = custom_env[0]  # idk how to not error out when unpacking

        match = re.match(SHA1_REGEX, commit, re.IGNORECASE)

        if match is None:
            output = subprocess.run(
                f"git ls-remote {url} {commit}".split(" "),
                capture_output=True,
                text=True,
            )
            print(output.stdout)
            commit = " ".join(output.stdout.split()).split(" ")[0]

        source = {
            "type": "git",
            "url": f"{url}",
            "commit": f"{commit}",
            "dest": f"{deps_dir}/{name}",
        }
        sources.append(source)

        env_dir = "../" + deps_dir

        if custom_env:
            build_options.append(f"-D{custom_env}={env_dir}/{name}")
        else:
            build_options.append(
                f"-DFETCHCONTENT_SOURCE_DIR_{name.upper()}={env_dir}/{name}"
            )

    print(yaml.dump(build_options))
    print(yaml.dump(sources))


if __name__ == "__main__":
    main()
