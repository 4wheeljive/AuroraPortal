import os
Import("env")

# Use 6 cores (leave headroom to avoid OOM with large IDF builds)
env.SetOption("num_jobs", 1)

# Enable ccache for ESP-IDF CMake build (where most compile time lives)
os.environ["IDF_CCACHE_ENABLE"] = "1"
os.environ["CCACHE_COMPILER_TYPE"] = "gcc"

# Wrap SCons-level compilers with ccache
cc = env["CC"]
cxx = env["CXX"]
if "ccache" not in cc:
    env.Replace(CC="ccache " + cc)
if "ccache" not in cxx:
    env.Replace(CXX="ccache " + cxx)
