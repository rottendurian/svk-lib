import subprocess
import platform
import json
import os


def setup():
    git_link = ["git", "clone",
                "https://github.com/Microsoft/vcpkg.git",
                "vcpkg"]

    if platform.system() == "Windows":
        clone = git_link
        script = ["cmd.exe", "/c", "vcpkg\\bootstrap-vcpkg.bat"]
    else:
        clone = git_link
        script = ["bash", "vcpkg/bootstrap-vcpkg.sh"]

    if os.path.isdir("vcpkg") is not True:
        subprocess.run(clone)

    subprocess.run(script + ["-disableMetrics"])

    vcpkg = {
        "dependencies": [
            "glslang",
            "glfw3"
        ]
    }

    with open("vcpkg.json", "w") as handle:
        json.dump(vcpkg, handle, indent=4)

    cmake_vcpkg = "set(CMAKE_TOOLCHAIN_FILE \"${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake\")"

    if os.path.isfile("CMakeLists.txt"):
        with open("CMakeLists.txt", "r") as handle:
            file_contents = handle.read()

            if not file_contents.startswith(cmake_vcpkg):
                with open("CMakeLists.txt", "w") as file:
                    file.write(cmake_vcpkg + "\n" + file_contents)

    else:
        with open("CMakeLists.txt", "w") as handle:
            handle.write(cmake_vcpkg)


if __name__ == "__main__":
    setup()
