#!/bin/bash

while true; do
    if [ -f ".config" ]
    then
        echo "Starting with .config"
        ./rscp2mqtt
    else
        echo "Starting with config.min"
        ./rscp2mqtt --config config.min
    fi
    if [ -z "$WATCHDOG" ]
    then
        break
    fi
done
