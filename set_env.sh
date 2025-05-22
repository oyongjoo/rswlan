if [ "$1" == "g" ]; then
export IMG="796933740782.dkr.ecr.eu-central-1.amazonaws.com/renesas/tin:latest"
else
export IMG="artifactory.global.renesas.com/shared-conn-docker-registry-local/renesas/rpi:v3.0.3"
fi