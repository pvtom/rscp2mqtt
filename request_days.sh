#!/bin/bash

if [ $# -ne 5 ]; then
    echo "Usage: $0 <broker> <port> <prefix> <start_date> <end_date>"
    exit 1
fi

broker=$1
port=$2
prefix=$3
start_date=$4
end_date=$5

if ! [[ $port =~ ^[0-9]+$ ]]; then
    echo "Invalid port number. Please provide a positive integer."
    exit 1
fi
if (($port < 1)) || (($port > 65535)); then
    echo "Invalid port number. Port must be between 1 and 65535."
    exit 1
fi
if ! [[ $start_date =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || ! [[ $end_date =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
    echo "Invalid date format. Please use YYYY-MM-DD."
    exit 1
fi
if ! date -d "$start_date" &> /dev/null; then
    echo "Invalid start date: $start_date"
    exit 1
fi
if ! date -d "$end_date" &> /dev/null; then
    echo "Invalid end date: $end_date"
    exit 1
fi

end_date=$(date -d "$end_date + 1 day" +"%Y-%m-%d")

while [[ $start_date < $end_date ]]; do
    echo "Requesting data for $start_date"
    mosquitto_pub -h $broker -p $port -t "$prefix/set/request/day" -m "$start_date"
    start_date=$(date -d "$start_date + 1 day" +"%Y-%m-%d")
done
