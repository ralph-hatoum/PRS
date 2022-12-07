import os
import socket
import time


with open("to_test.txt", "r") as f:
    to_test_str = [i.split(" ")[1:] for i in f.readlines()]


command = "./server 6000"

for param_list in to_test_str:
    to_add = param_list[-1][:-1]
    param_list.pop()
    param_list.append(to_add)

timeouts, windows, seg_sizes = to_test_str[0], to_test_str[1], to_test_str[2]

commands = []

for timeout in timeouts:
    for window in windows:
        for seg_size in seg_sizes:
            com = command+" "+timeout+" "+window+" "+seg_size
            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.connect(("127.0.0.1", 6789))
            print("Starting a test - launching command "+com)
            client.send("START".encode())
            print(timeout+" "+window+" "+seg_size)
            client.send((timeout+" "+window+" "+seg_size).encode())
            client.close()
            os.system(com)
            time.sleep(1)
                





    

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(("127.0.0.1", 6789))
client.send("DONE".encode())
client.close()

