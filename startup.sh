#!/bin/bash

if [ -f ".config" ]
then
    echo "Starting with .config"
    ./rscp2mqtt
else
    echo "Starting with config.min"
    ./rscp2mqtt --config config.min
fi
