#!/bin/bash
set -e  # If it fails, it stops immediately
cd "$(dirname "$0")"
source set_env.sh $1
echo "IMAGE: $IMG"
echo "BOARD: $2"
echo "CHIP: $3"
echo "INTERFACE: $4"
echo "REVISION: $5"
echo "COMMAND: $6"
export IMAGE=${IMG}
export BOARD=$2
export CHIP=$3
export INTERFACE=$4
export REVISION=$5
export COMMAND=$6
echo "Exported IMAGE: $IMAGE"
echo "Exported COMMAND: $COMMAND"
docker compose up $COMMAND || { echo "Docker compose fail!!"; exit 1; }