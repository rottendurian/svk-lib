import os

if __name__ == "__main__":
    os.chdir("svklib/dep/glslang/")
    with open("update_glslang_sources.py") as handle:
        exec(handle.read())
