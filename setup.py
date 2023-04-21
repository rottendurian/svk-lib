import subprocess
import platform

def setup():
    if platform.system() == "Windows":
        script = ["cmd.exe", "/c", "vcpkg\\bootstrap-vcpkg.bat"]
    else:
        script = ["bash", "vcpkg/bootstrap-vcpkg.sh"]

    subprocess.run(script)



if __name__ == "__main__":
    setup()


