import os
import socket
import time


'''with open("to_test.txt", "r") as f:
    to_test_str = [i.split(" ")[1:] for i in f.readline@s()]
'''

command = "./server 6000"
'''
for param_list in to_test_str:
    to_add = param_list[-1][:-1]
    param_list.pop()
    param_list.append(to_add)
    '''

timeouts, windows, seg_sizes = [str(i/1000) for i in range(0,501,50)], [str(i) for i in range(1,101)], [str(i) for i in range(1000, 1501,20)]

commands = []

cpt = 0

for timeout in timeouts:
    for window in windows:
        for seg_size in seg_sizes:
            com = command+" "+seg_size+" "+window+" "+timeout
            cpt += 1
            print(com)
            
            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.connect(("127.0.0.1", 6789))
            print("Starting a test - launching command "+com)
            client.send("START".encode())
            print(timeout+" "+window+" "+seg_size)
            client.send((timeout+" "+window+" "+seg_size).encode())
            client.close()
            os.system(com)
            time.sleep(1)
            

#estimated_time = cpt*10  
#print("Temps estimé pour faire les tests : ", estimated_time/3600)





    

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(("127.0.0.1", 6789))
client.send("DONE".encode())
client.close()

