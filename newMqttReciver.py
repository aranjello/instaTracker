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

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("requestData")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))
    requestQueue.put(msg.payload.decode('ascii'))


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set("agoodman", password="Dinomug96")
def waitForInternet():
    while True:
        try:
            client.connect("10.0.0.75", 1883, 60)
            print("connected")
            return
        except:
            pass

waitForInternet()
client.loop_start()
now = datetime.now()
current_time = now.strftime("%M:%S")
while True:
    now = datetime.now()
    if(now.strftime("%M:%S") != current_time):
        current_time = now.strftime("%M:%S")
        print(current_time)
    if(not(requestQueue.empty())):
        try:
            out = requestQueue.get()
            profile = instaloader.Profile.from_username(L.context, out).followers                             
            print(out + " " + str(profile))
            client.publish(out, payload=str(profile), qos=0, retain=False)
        except:
            client.publish(out, payload="error",qos=0,retain=False)
        
            
    
