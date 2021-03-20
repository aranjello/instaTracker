import paho.mqtt.client as mqtt
import queue
import instaloader
import time
from datetime import datetime

# Get instance
L = instaloader.Instaloader()

# Optionally, login or load session
#L.login("jimsamson214@gmail.com", "JimboJammer") 
requestQueue = queue.Queue()
alreadyIn = []
alreadyList = []
messageBack = True
lastTime = datetime.now()
lastSent = ""

def sendData(out,insta=True):
    try:
        if(insta):
            print(str(datetime.now()) + " " + "getting data for :" + out)
            if(out in alreadyIn):
                alreadyIn.remove(out)
            profile = instaloader.Profile.from_username(L.context, out).followers                             
            print(str(datetime.now()) + " " + out + " " + str(profile))
            client.publish(out, payload=str(profile), qos=0, retain=False)
            lastSent = out
        else:
            client.publish(out,payload="recieved", qos=0, retain=False)
        lastTime = datetime.now()
        messageBack = False
    except:
        client.publish(out, payload="error",qos=0,retain=False)
        lastTime = datetime.now()
        messageBack = False

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print(str(datetime.now()) + " " + "Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("requestData")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))
    if(msg.payload.decode('ascii') == "gotMessage"):
        messageBack = True
    else:
        if(not(msg.payload.decode('ascii') in alreadyIn)):
            if(not(msg.payload.decode('ascii')) in alreadyList):
                print(str(datetime.now())+str(msg.payload) + " " + "added to queue and list")
                alreadyList.append(msg.payload.decode('ascii'))
                sendData(msg.payload.decode('ascii'))
            else:
                print(str(datetime.now())+str(msg.payload) + " " + "added to queue")
                alreadyIn.append(msg.payload.decode('ascii'))
                requestQueue.put(msg.payload.decode('ascii'))
        else:
            print(str(datetime.now()) + " " + "already in queue")
        sendData(msg.payload.decode('ascii'),insta=False)
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set("agoodman", password="Dinomug96")
def waitForInternet():
    while True:
        try:
            client.connect("goodtimes.mywire.org", 1883, 60)
            print(str(datetime.now()) + " " + "connected")
            return
        except:
            pass

waitForInternet()
client.loop_start()
now = datetime.now()
waitTime = datetime.now()
while True:
    now = datetime.now()
    if(not(messageBack) and ((lastTime - now).seconds > 10)):
        sendData(lastSent)
    diffTime = now.minute - waitTime.minute if (now.hour == waitTime.hour) else now.minute-waitTime.minute + 60
    if(diffTime > 5 and not(requestQueue.empty())):
        waitTime = now
        out = requestQueue.get()
        sendData(out)
        
            
    
