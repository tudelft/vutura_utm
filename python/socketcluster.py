#!/usr/bin/env python3

from socketclusterclient import Socketcluster

import logging
import nanomsg
import json
import vutura_common_pb2
import dateutil.parser

logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

traffic_pub = nanomsg.Socket(nanomsg.PUB)
traffic_pub.bind('tcp://127.0.0.1:8340'.encode('utf-8'))

def onconnect(socket):
    logging.info("on connect got called")


def ondisconnect(socket):
    logging.info("on disconnect got called")


def onConnectError(socket, error):
    logging.info("On connect error got called")


def onSetAuthentication(socket, token):
    logging.info("Token received " + token)
    socket.setAuthtoken(token)


def onAuthentication(socket, isauthenticated):
    logging.info("Authenticated is " + str(isauthenticated))
    #socket.subscribeack('nearby:5180788357881397248', onSubAck)	
    socket.subscribeack('adsb', onSubAck)
    socket.onchannel('adsb', onTraffic)

def onTraffic(key, object):
    try:
        d = json.dumps(object)
        j = json.loads(d)
        for item in j["data"]:
            traffic_message = vutura_common_pb2.TrafficInfo()
            traffic_message.unique_id = item["movement"]
            traffic_message.aircraft_id = item["identification"]
            recorded_time = dateutil.parser.parse(item["timestamp"])
            traffic_message.timestamp = round(recorded_time.timestamp() * 1000)
            traffic_message.recorded_time = round(recorded_time.timestamp() * 1000)
            traffic_message.lat = round(item["location"]["latitude"] * 1e7)
            traffic_message.lon = round(item["location"]["longitude"] * 1e7)
            traffic_message.alt = round(item["altitude"]["value"] * 1e3)
            traffic_message.groundspeed = 0
            traffic_message.heading = 0
            traffic_pub.send(traffic_message.SerializeToString())
            if (item["identification"] == "flight|G8ywkEn8JZ7N6hg2nywOXg8voKulqoa72wDYZyOHWBPanvQNDZk5"):
                print(json.dumps(item, indent=4))
    except:
        print("error parsing json")

    #print("Got data\n\n\n", object, "\n\nfrom\n", key)

def onSubAck(channel, error, object):
    if error is '':
        print("Subscribed to channel")
    else:
        print(error)
    
    
if __name__ == "__main__":
    socket = Socketcluster.socket("wss://vutura.unifly.tech:443/socketcluster/")
    socket.setBasicListener(onconnect, ondisconnect, onConnectError)
    socket.setAuthenticationListener(onSetAuthentication, onAuthentication)

    # read token from file
    f = open('/home/bart/unifly/token.txt', 'r')
    token = f.readline()

    onSetAuthentication(socket, token)

    # listen to channel

    socket.connect()
