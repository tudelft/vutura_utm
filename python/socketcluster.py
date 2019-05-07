#!/usr/bin/env python3

from socketclusterclient import Socketcluster

import logging

logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)


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
    print("Got data\n\n\n", object, "\n\nfrom\n", key)

def onSubAck(channel, error, object):
    if error is '':
        print("Subscribed to channel")
    else:
        print(error)
    
    
if __name__ == "__main__":
    socket = Socketcluster.socket("wss://vutura.unifly.tech/socketcluster/")
    socket.setBasicListener(onconnect, ondisconnect, onConnectError)
    socket.setAuthenticationListener(onSetAuthentication, onAuthentication)

    # read token from file
    f = open('/home/bart/unifly/token.txt', 'r')
    token = f.readline()

    onSetAuthentication(socket, token)

    # listen to channel

    socket.connect()
