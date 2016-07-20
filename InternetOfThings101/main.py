#!/usr/bin/python

import dweepy
import paho.mqtt.client as paho
import psutil
import pywapi
import signal
import sys
import time
import uuid
import json
import pyupm_grove as grove

from flask import Flask
from flask_restful import Api, Resource
from threading import Thread

DeviceID = "Jair01"

# Create the relay switch object using GPIO pin 0
relay = grove.GroveRelay(5)

# Create the light sensor object using AIO pin 0
light = grove.GroveLight(0)

class SensorRestApi(Resource):
    def get(self):
        data = 'Sensor data: %s' % functionDataSensor()
        return data

def functionFlaskWorker():
    app = Flask(__name__)
    api = Api(app)
    api.add_resource(SensorRestApi, '/sensor')
    app.run(host='0.0.0.0', debug=True, use_reloader=False)

def functionApiWeather():
    data = pywapi.get_weather_from_weather_com('MXJO0043', 'metric')
    message = data['location']['name']
    message = message + ", Temperature " + data['current_conditions']['temperature'] + " C"
    message = message + ", Atmospheric Pressure " + data['current_conditions']['barometer']['reading'][:-3] + " mbar"
    return message

def functionDataActuator(status):
    if status=="1":
        relay.on()
    else:
        relay.off()
    print "Data Actuator Status %s" % status

def functionDataActuatorMqttOnMessage(mosq, obj, msg):
    print "Data Sensor Mqtt Subscribe Message!"
    functionDataActuator(msg.payload)

def functionDataActuatorMqttSubscribe():
    mqttclient = paho.Client()
    mqttclient.on_message = functionDataActuatorMqttOnMessage
    mqttclient.connect("test.mosquitto.org", 1883, 60)
    mqttclient.subscribe("IoT101/" + DeviceID + "/DataActuator", 0)
    while mqttclient.loop() == 0:
        pass

def functionDataSensor():
    data = light.value()
    return data

def functionDataSensorMqttOnPublish(mosq, obj, msg):
    print "Data Sensor Mqtt Published!"

def functionDataSensorMqttPublish():
    mqttclient = paho.Client()
    mqttclient.on_publish = functionDataSensorMqttOnPublish
    mqttclient.connect("test.mosquitto.org", 1883, 60)
    while True:
        data = functionDataSensor()
        topic = "IoT101/" + DeviceID + "/DataSensor"
        mqttclient.publish(topic, data)
        time.sleep(1)

def functionServicesDweet():
    while True:
        time.sleep(5)
        dweepy.dweet_for('InternetOfThings101JairDweet', {'Status':'1'})
        print dweepy.get_latest_dweet_for('InternetOfThings101JairDweet')
        time.sleep(5)
        dweepy.dweet_for('InternetOfThings101JairDweet', {'Status':'0'})
        print dweepy.get_latest_dweet_for('InternetOfThings101JairDweet')

def functionDataSensorWatsonOnPublish(mosq, obj, msg):
    print "Data Sensor Watson Published!"

def functionPlatformWatson():
    macAddress = hex(uuid.getnode())[2:-1]
    macAddress = format(long(macAddress, 16),'012x')
    #remind the user of the mac address further down in code (post 'connecitng to QS')

    #Set the variables for connecting to the Quickstart service
    organization = "quickstart"
    deviceType = "iotsample-gateway"
    broker = ""
    topic = "iot-2/evt/status/fmt/json"
    username = ""
    password = ""

    print("MAC address: " + macAddress)

    #Creating the client connection
    #Set clientID and broker
    clientID = "d:" + organization + ":" + deviceType + ":" + macAddress
    broker = organization + ".messaging.internetofthings.ibmcloud.com"

    mqttclient = paho.Client(clientID)
    mqttclient.on_publish = functionDataSensorWatsonOnPublish
    mqttclient.connect(broker, 1883, 60)

    while True:
        data = functionDataSensor()
        msg = json.JSONEncoder().encode({"d":{"SensorData":data}})
        mqttclient.publish(topic, msg)
        time.sleep(2)

def functionSignalHandler(signal, frame):
    sys.exit(0)

if __name__ == '__main__':

    signal.signal(signal.SIGINT, functionSignalHandler)

    threadflaskworker = Thread(target=functionFlaskWorker)
    threadflaskworker.start()

    threadmqttpublish = Thread(target=functionDataSensorMqttPublish)
    threadmqttpublish.start()

    threadmqttsubscribe = Thread(target=functionDataActuatorMqttSubscribe)
    threadmqttsubscribe.start()

    threadservicesdweet = Thread(target=functionServicesDweet)
    threadservicesdweet.start()

    threadplatformwatson = Thread(target=functionPlatformWatson)
    threadplatformwatson.start()

    while True:
        print "Hello Internet of Things 101 from %s" % DeviceID
        print "Data Sensor: %s " % functionDataSensor()
        print "API Weather: %s " % functionApiWeather()
        time.sleep(5)

del relay
del light

# End of File
