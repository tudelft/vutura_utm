#!/usr/bin/env python3

from socketclusterclient import Socketcluster

import logging
import nanomsg
import json
import vutura_common_pb2
import dateutil.parser

import requests
  
logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

traffic_pub = nanomsg.Socket(nanomsg.PUB)
traffic_pub.bind('tcp://0.0.0.0:8340')

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
            traffic_message.groundspeed = round(item["aircraftData"]["groundSpeed"] * 1e3 * 0.5144447)
            traffic_message.heading = round(item["heading"]["trueHeading"])

            if traffic_message.lat > 515000000 and traffic_message.lat < 525000000 and traffic_message.lon > 40000000 and traffic_message.lon < 50000000:
                traffic_pub.send(traffic_message.SerializeToString())
            #if (item["identification"] == "flight|G8ywkEn8JZ7N6hg2nywOXg8voKulqoa72wDYZyOHWBPanvQNDZk5"):
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


    print("Try to get token")


    headers = {'content-type': 'application/x-www-form-urlencoded',
                              'authorization': 'Basic dHVkZWxmdFZ1dHVyYUNvbm5lY3Q6dHVEZWxmdDIwMTkwM1Z1dHVyYQ==',
                                                        'accept': 'application/json'}
    payload = "username=b.j.m.m.slinger@tudelft.nl&password=********************************************************************************************&grant_type=password"

    r = requests.post("https://vutura.unifly.tech/oauth/token", data=payload, headers=headers)

    token = (r.json()["access_token"])
    print(token)

    # write to file
    token_file = open("~/unifly/token.txt", "w")
    token_file.write(token)
    token_file.close()

    socket = Socketcluster.socket("wss://vutura.unifly.tech:443/socketcluster/")
    socket.setBasicListener(onconnect, ondisconnect, onConnectError)
    socket.setAuthenticationListener(onSetAuthentication, onAuthentication)

    # read token from file
    f = open('~/unifly/token.txt', 'r')
    token = f.readline().rstrip()

    onSetAuthentication(socket, token)

    # listen to channel

    socket.connect()

