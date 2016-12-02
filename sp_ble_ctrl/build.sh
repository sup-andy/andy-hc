#!/bin/sh
TOPDIR=$(cd ./../../../../../../ && /bin/pwd)
. ${TOPDIR}/tools/build_tools/profile_member01_wdl.sh
. ${TOPDIR}/tools/build_tools/Path.sh
APPS_NAME="sp_ble_ctrl"

parse_args $@

if [ $BUILD_CLEAN -eq 1 ]; then
    if [ -e Makefile -o -e makefile ]; then
        make clean
        rm -f .built
        rm -f .config*
        rm -f .dep*
        rm -f .configured*
        rm -f .prepared*
        rm -f .version*
        find . -name *.o | xargs rm -f
    fi
    [ ! $BUILD_CONFIGURE -eq 1 ] && exit 0
fi

if [ -x patch.sh ]; then
    ./patch.sh
fi

make CC="${TOOL_CC}" TOOLCHAIN_PATH="${TOOLCHAIN_DIR_BASE}" CROSS_COMPILE="${CROSS_COMPILER_PREFIX}" ARCH="${TARGET_ARCH}" EXTRA_CFLAGS="${TARGET_CFLAGS} ${TARGET_EXTRACFLAGS} ${MGR_EXTRA_CFLAGS}" EXTRA_LDFLAGS="${TARGET_LDFLAGS}" COMMON_PATH="${USER_PRIVATE_DIR}/common" INSTALL_PATH="${BUILD_ROOTFS_DIR}" all
build_error_check $?


# COPY the binary file to public directory
if [ -d ${MEMBER_PUBLIC_DIR}/${APPS_NAME} ]; then
    ${COPY} ${MEMBER_PRIVATE_DIR}/${APPS_NAME}/sp_ble_ctrl ${MEMBER_PUBLIC_DIR}/${APPS_NAME}/
    build_error_check $?
fi

