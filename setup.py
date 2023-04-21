import os 
import subprocess
import platform

def setup():
    if platform.system() == "Windows":
        script = "vcpkg/bootstrap-vcpkg.bat"
    else:
        script = "vcpkg/bootstrap-vcpkg.sh"
    #bootstrap-vcpkg
    subprocess.run(script)
    
    subprocess.run(["vcpkg/vcpkg", "install", "glslang"])
    subprocess.run(["vcpkg/vcpkg", "install", "glfw3"])


if __name__ == "__main__":
    setup()


