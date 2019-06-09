import socket
import time
import threading

routerTable = {}
init_table = {
    'A':{'A':0, 'B':1, 'C':5, 'D':10000, 'E':10000},
    'B':{'A':1, 'B':0, 'C':2, 'D':1, 'E':10000},
    'C':{'A':5, 'B':2, 'C':0, 'D':10000, 'E':3},
    'D':{'A':10000, 'B':1, 'C':10000, 'D':0, 'E':10000},
    'E':{'A':10000, 'B':10000, 'C':3, 'D':10000, 'E':0}}
port_table = {'A':52001, 'B':52002, 'C':52003, 'D':52004, 'E':52005}

class Node:
    def __init__(self, ID):
        self.ID = ID
        self.distance = {}
        self.nextHop = {}

    def setDistance(self, d_node, distance):
        self.distance[d_node] = distance

    def setNextHop(self, d_node, nextHop):
        self.nextHop[d_node] = nextHop

def initRouter():
    for s_node in init_table:
        node = Node(s_node)
        for d_node in init_table[s_node]:
            node.setDistance(d_node,init_table[s_node][d_node])
            node.setNextHop(d_node,d_node)
        routerTable[s_node] = node

def updateRouter():
    update = True
    while update:
        update = False
        for s_node in routerTable:
            for d_node in routerTable:
                for m_node in routerTable:
                    if routerTable[s_node].distance[d_node] > init_table[s_node][m_node] + routerTable[m_node].distance[d_node]:
                        routerTable[s_node].distance[d_node] = init_table[s_node][m_node] + routerTable[m_node].distance[d_node]
                        routerTable[s_node].nextHop[d_node] = m_node
                        update = True

def routeThread(ID):
    socket.setdefaulttimeout(5)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    host = socket.gethostname()
    s.bind((host,port_table[ID]))
    #print(ID,"thread success\n")

    while True:
        try:
            recvDatab,recvAdrr = s.recvfrom(1024)
            recvData = recvDatab.decode().split()
            if recvData[0] == "quit":
                break;
            destName = routerTable[ID].nextHop[recvData[1]]
            destPort = port_table[destName]

            printStr = "\n" + ID + " received packet.\n"
            printStr = printStr + "source:" + recvData[0] + " destination:" + recvData[1] + " ttl:" + recvData[2] + "\n"
            if destName == ID:
                printStr = printStr + "packet arrived " + ID + "\n"
            elif int(recvData[2]) == 0:
                printStr = printStr + "packet dropped\n"
            else:
                printStr = printStr + "nextHop:" + destName + "\n"
                recvData[2] = str(int(recvData[2]) - 1)
                sendData = recvData[0] + " " + recvData[1] + " " + recvData[2]
                s.sendto(sendData.encode(),(host,destPort))
            print(printStr)
        except socket.timeout as e:
            pass

    s.close()

####构造静态路由表###
initRouter()
updateRouter()
for node in routerTable:
    print("---------------Router %s---------------"%node)
    print("           Distance            Next-hop")
    for d_node in routerTable[node].distance:
        print("  %s            %d                 %s    "%(d_node, 
              routerTable[node].distance[d_node], 
              routerTable[node].nextHop[d_node]))
    print("\n")
###多线程模拟路由器结点，收发数据###
threads = []
for node in routerTable:
    t = threading.Thread(target=routeThread,args=(node,))
    threads.append(t)
for t in threads:
    t.start()

packet = "D E 3"        # source destination ttl
skt = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
host = socket.gethostname()
skt.bind((host,52000))
skt.sendto(packet.encode(),(host,port_table['D']))
time.sleep(10)   # 等待packet转发完成，关闭所有线程
for node in port_table:
    skt.sendto("quit".encode(),(host,port_table[node]))
skt.close()
for t in threads:
    t.join()
input("Please enter...")
