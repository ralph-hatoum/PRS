import os
import subprocess
import threading
import shlex
import time

commands = ["./server 6000&", "./client localhost 6000"]

for command in commands:
    subprocess.run(shlex.split(command))

