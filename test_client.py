

from cgi import test
from ipaddress import ip_address
import socket
import os
import time

ip= "134.214.202.43"
port = "6000"
file_name = "pdf.pdf"

command = "time ./client1 "+ip+" "+port+" "+file_name+" > current_test.txt"

serveur = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

serveur.bind(('', 6789))
serveur.listen(1)
print("Server started and listening -- awaiting instructions to launch client")
testing = True
f = open("results.txt","w")
while testing:
    client, adresseClient = serveur.accept()
    donnees = client.recv(5)
    print(donnees)
    time.sleep(1)
    
    if donnees==b"START":
        label = client.recv(50)
        print(label)
        start=time.time()
        os.system(command)
        end = time.time()
        t = end-start
        f.write(label.decode()+" "+t)


    elif donnees==b"DONE":
        testing = False

serveur.close()

