#!/bin/bash

TRUE=1
FALSE=0

chip="undefined"
action="undefined"
interface="undefined"
revision="undefined"

CHIP_IS_VALID=$FALSE

############################################################################################################
# Function to display help message
############################################################################################################
help() {
	echo "Usage: KDIR=path/to/kernel/dir ENV_SETUP_PATH=path/to/sdk $0 [-a | --action] action [-c | --chip] chip [-i | --interface] interface [-r | --revision] revision"
	echo ""
	echo "Commands:"
	echo "  -h | --help               Show this help message and exit"
	echo "  -a | --action             Set action"
	echo "  -c | --chip               Set chip type"
    echo "  -i | --interface          Set interface type"
    echo "  -r | --revision           Set board revision"
	echo ""
    echo "Actions:"
	echo "  build"
    echo "  clean"
    echo "  debug"
	echo ""
	echo "Chips:"
	echo "  vanadium"
    echo "  tin"
    echo "Interface:"
    echo "  sdio"
    echo "  spi"
	echo "Revision:"
	echo "  aa"
    echo "  ba"
	echo ""
	echo "Example:"
	echo "  KDIR=/home/rzg2l-5.10.158 ENV_SETUP_PATH=/opt/poky/3.1.21/environment-setup-aarch64-poky-linux $0 -a build -c tin -i sdio -r aa"
	echo "  $0 -a build -c tin -i sdio"
	echo "  $0 --action build --chip vanadium --interface sdio --revision aa"
}

############################################################################################################
# Argument parser function
############################################################################################################
argparse() {
    if [ $? != 0 ]; then
        help
        exit 1;
    fi

    eval set -- "$1"
    while :
        do
            case $1 in
                -a | --action)
                    action=$2
                    shift 2
                    ;;
                -c | --chip)
                    chip=$2
                    shift 2
                    ;;
                -i | --interface)
                    interface=$2
                    shift 2
                    ;;
                -r | --revision)
                    revision=$2
                    shift 2
                    ;;
                -h | --help)
                    help
                    exit 0
                    ;;
                --)
                    shift
                    break
                    ;;
            esac
        done
}

############################################################################################################
# Chip verification function
############################################################################################################
verify_chip() {
    case "$1" in
        "tin")
            export DRV_VER=6.1.5.99.5
            export CONFIG_RSWLAN_CHIP_TYPE=RRQ61000
            CHIP_IS_VALID=$TRUE
            ;;
        "vanadium")
            export DRV_VER=7.1.5.99.3
            export CONFIG_RSWLAN_CHIP_TYPE=RRQ61400
            CHIP_IS_VALID=$TRUE
            ;;
    esac
}

verify_interface() {
    case "$1" in
        "sdio")
            export CONFIG_RSWLAN_SDIO=m
            ;;
        "spi")
            export CONFIG_RSWLAN_SPI=m
            ;;
    esac
}

verify_revision() {
    case "$1" in
        "aa")
            export CONFIG_RSWLAN_CHIP_REVISION=AA
            ;;
        "ba")
            export CONFIG_RSWLAN_CHIP_REVISION=BA
            ;;
    esac
}

############################################################################################################
# Main body
############################################################################################################
echo "Running $0"

options=$(getopt -o ha:c:i:r: --long help,action:,chip:,interface:,revision: -- "$@")
argparse "$options"

echo "############################################################################################################"
echo "Chip type is: ${chip}"
echo "Action type is: ${action}"
echo "Interface type is: ${interface}"
echo "Revision is: ${revision}"
echo "KDIR : ${KDIR}"
echo "ENV_SETUP_PATH : ${ENV_SETUP_PATH}"
echo "############################################################################################################"

verify_chip $chip
verify_interface $interface
verify_revision $revision

if [ "${ENV_SETUP_PATH}" == "" ]; then
  echo "Warning: ENV_SETUP_PATH is not set. Using default value: 3.1.21"
  export ENV_SETUP_PATH=/opt/poky/3.1.5/environment-setup-aarch64-poky-linux
fi

if [ "$KDIR" == "" ]
then
  echo "Warning: KDIR is not set. Using default value: /home/rzg2l-5.10.158"
	export KDIR=/home/rzg2l-5.10.158
fi

source ${ENV_SETUP_PATH}

case "$action" in
    "build")
        if [ $CHIP_IS_VALID -ne $FALSE ]; then
            export CONFIG_RSWLAN_DEBUG_BUILD=n
            echo $CC
            make -j4
        else
            echo "Error! Chip configuration is not valid."
            help;
            exit 1
        fi
        ;;
    "clean")
        make clean
        ;;
    "debug")
        if [ $CHIP_IS_VALID -ne $FALSE ]; then
            export CONFIG_RSWLAN_DEBUG_BUILD=y
            echo $CC
            make -j4
        else
            echo "Error! Chip configuration is not valid."
            help;
            exit 1
        fi
        ;;
    *)
        help
        exit 1
        ;;
esac
