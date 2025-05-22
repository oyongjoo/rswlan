#!/bin/bash

TRUE=1
FALSE=0

board="undefined"
chip="undefined"
action="undefined"
interface="undefined"
revision="undefined"

BOARD_IS_VALID=$FALSE

BUILD_DIR="build/linux"

############################################################################################################
# Function to display help message
############################################################################################################
help() {
	echo "Usage: $0 [-a | --action] action [-b | --board] board [-c | --chip] chip [-i | --interface] interface [-r | --revision] revision"
	echo ""
	echo "Commands:"
	echo "  -a | --action             Action to do"
	echo "  -h | --help               Show this help message and exit"
	echo "  -b | --board              Set board type"
	echo "  -c | --chip               Set chip type"
	echo "  -i | --interface          Set interface type"
	echo "  -r | --revision           Set revision type"
	echo ""
    echo "Actions:"
	echo "  build"
    echo "  clean"
    echo "  debug"
	echo ""
	echo "Boards:"
	echo "  rzg2l            build rzg2l v3.0.3"
    echo "  rzg2l_306        build rzg2l v3.0.6"
    echo "  rzg3s            build rzg3s"
	echo "  geniatech        build geniatech"
    echo "  rpi_legacy       build Raspberry PI4 or earlier"
    echo "  rpi_pi5          build Raspberry PI5"
	echo "  custom           build with custom SDK. Requires Environment Variables:"
	echo "                   ENV_SETUP_PATH   Path to setup build environment script"
	echo "                   KDIR             Kernel directory path"
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
	echo "  export KDIR=/home/rzg2l-5.10.158 ENV_SETUP_PATH=/opt/poky/3.1.21/environment-setup-aarch64-poky-linux $0 -b geniatech -c tin -i sdio -p custom -r aa"
	echo "  $0 -a build -b geniatech -c tin -i sdio -r aa"
	echo "  $0 --action build --board geniatech --chip vanadium --interface spi --revision aa"
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
                -b | --board)
                    board=$2
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
# Board verification function
############################################################################################################
veryfy_board() {
    case "$1" in
        "rzg2l")
            export KDIR=/home/rzg2l-5.10.158
            export ENV_SETUP_PATH=/opt/poky/3.1.31/environment-setup-aarch64-poky-linux
            BOARD_IS_VALID=$TRUE
            ;;

        "rzg2l_306")
            export KDIR=/home/rzg2l-5.10.201
            export ENV_SETUP_PATH=/opt/poky/3.1.21/environment-setup-aarch64-poky-linux
            BOARD_IS_VALID=$TRUE
            ;;

        "geniatech")
            export KDIR=/home/developer/work/rzg_vlp/build/tmp/work/smarc_rzg2l-poky-linux/linux-renesas/5.10.201-cip41+gitAUTOINC+0030c60827-r1/linux-smarc_rzg2l-standard-build
            export ENV_SETUP_PATH=/opt/poky/3.1.31/environment-setup-aarch64-poky-linux
            BOARD_IS_VALID=$TRUE
            ;;

        "rpi_legacy")
            export ENV_SETUP_PATH=/opt/rpi/env_toolchain_rpi_aarch64
            export KDIR=/home/developer/work/raspberrypi4
            BOARD_IS_VALID=$TRUE
            ;;

        "rpi_pi5")
            export ENV_SETUP_PATH=/opt/rpi/env_toolchain_rpi_aarch64
            export KDIR=/home/developer/work/raspberrypi5
            BOARD_IS_VALID=$TRUE
            ;;

        "rzg3s")
            export KDIR=/home/developer/work/renesas_rz_linux/build/tmp/work/smarc_rzg3s-poky-linux/linux-renesas/5.10.145-cip17+gitAUTOINC+bbd33951a0-r1/linux-smarc_rzg3s-standard-build
            export ENV_SETUP_PATH=/opt/poky/3.1.21/environment-setup-aarch64-poky-linux
            BOARD_IS_VALID=$TRUE
            ;;

        "custom")
            if [ -z "${KDIR}" ]; then
                echo "Error: KDIR global variable is not set"
                BOARD_IS_VALID=$FALSE
            fi
            if [ -z "${ENV_SETUP_PATH}" ]; then
                echo "ERROR: ENV_SETUP_PATH global variable is not set"
                BOARD_IS_VALID=$FALSE
            fi
            ;;
    esac
}

############################################################################################################
# Main body
############################################################################################################
echo "Running $0"

options=$(getopt -o ha:b:c:i:r: --long help,action:,board:,chip:,interface:,revision: -- "$@")
argparse "$options"

echo "############################################################################################################"
echo "Chip type is: ${chip}"
echo "Board type is: ${board}"
echo "Action type is: ${action}"
echo "Revision is: ${revision}"
echo "############################################################################################################"

veryfy_board $board

case "$action" in
    "build")
        if [ $BOARD_IS_VALID -ne $FALSE ]; then
            make
            cd ${BUILD_DIR}
            KDIR=${KDIR} ENV_SETUP_PATH=${ENV_SETUP_PATH} ./build.linux.sh --action build --chip ${chip} --interface ${interface} --revision ${revision}
        else
            echo "Error! Check board configuration."
            help;
            exit 1
        fi
        ;;
    "clean")
        make clean
        ;;
    "debug")
        if [ $BOARD_IS_VALID -ne $FALSE ]; then
            export CONFIG_RSWLAN_DEBUG_BUILD=n
            make
            cd ${BUILD_DIR}
            KDIR=${KDIR} ENV_SETUP_PATH=${ENV_SETUP_PATH} ./build.linux.sh --action debug --chip ${chip} --interface ${interface} --revision ${revision}
        else
            echo "Error! Check board configuration."
            help;
            exit 1
        fi
        ;;
    *)
        help
        exit 1
        ;;
esac

