import os
import subprocess
import sys

#c compiler, takes c or h endings
#usage is compile.py output.exe (compiles all .c files in directory)
#compile.py file.c output.exe   (compiles just one .c file)


compiler = "gcc"
flags = ["-std=c99"]

if __name__ == "__main__":
    if(len(sys.argv) == 2):
        cfiles = [f for f in os.listdir(".") if f.endswith(".c")]

        command = [compiler] + flags + cfiles + ["-o", sys.argv[1]]
        print(" ".join(command))
        if(not subprocess.call(command)):
            subprocess.call([sys.argv[1]])
    elif(len(sys.argv) == 3):
        command = [compiler] + flags + [sys.argv[1], "-o", sys.argv[2]]
        print(" ".join(command))
        if(not subprocess.call(command)):
            subprocess.call([sys.argv[2]])
    else:
        print("Incorrect number of arguments")
