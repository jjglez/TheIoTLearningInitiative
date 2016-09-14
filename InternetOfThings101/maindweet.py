#!/usr/bin/python

import dweepy
import time

def functionServicesDweet():
    while True:
        time.sleep(5)
        dweepy.dweet_for('InternetOfThings101Dweet', {'Status':'1'})
        print dweepy.get_latest_dweet_for('InternetOfThings101Dweet')
        time.sleep(5)
        dweepy.dweet_for('InternetOfThings101Dweet', {'Status':'0'})
        print dweepy.get_latest_dweet_for('InternetOfThings101Dweet')

if __name__ == '__main__':

    functionServicesDweet()

# End of File
