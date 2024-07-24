#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
#make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- clean
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps her
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} INSTALL_PATH=${OUTDIR}/ -j$(nproc) all
fi


echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/
echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs && cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
if [ ! -e ${OUTDIR}/busybox/busybox ]; then
    make -j$(nproc) ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
fi
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- install
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
#"program interpreter"
sysroot=$(${CROSS_COMPILE}gcc --print-sysroot)
prog_interpreter=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | tr -d "[]")
delem=":"
path_to_interpreter=${prog_interpreter#*$delem}
interpreter=$(basename $path_to_interpreter | tr -d " ")
path_prefix_interpret=$(dirname $path_to_interpreter)
final_path=${OUTDIR}/rootfs/$path_prefix_interpret
echo $final_path
sudo cp $(find $sysroot -name $interpreter) $final_path

#"Shared library"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | tr -d "[]" >tmp.txt
int=0
echo $sysroot

while read -r line
do
    shared=$(echo ${line#*$delem} | tr -d " ")
    for path_to_shared in $(find $sysroot -name $shared) ;  do
        echo 
        realtive=${path_to_shared#*$sysroot}
        tmp_path=$(dirname $realtive)
        final_path=${OUTDIR}/rootfs/$tmp_path
        if [ ! -d "${final_path}" ] ; then
            echo final_path:$final_path
            mkdir -p $final_path
        fi
        echo "copy from $path_to_shared  to $final_path"
        sudo cp $path_to_shared $final_path
    done
done < tmp.txt

rm tmp.txt

# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}/
echo "cleaning the writer app ..."
make CROSS_COMPILE=${CROSS_COMPILE} clean
echo "Building the writer app ..."
make CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
cp writer ${OUTDIR}/rootfs/home/
# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
 #finder.sh, conf/username.txt, conf/assignment.txt and finder-test.sh
if [ ! -e ${OUTDIR}/rootfs/home/finder-test.sh ]; then
    echo " copy finder.sh, conf/username.txt, conf/assignment.txt and finder-test.sh into rootfs/home ..."
    cp finder.sh finder-test.sh ${OUTDIR}/rootfs/home
    cp -r ../conf ${OUTDIR}/rootfs/home/
    sed -i -e 's/..\/conf\/assignment.txt/conf\/assignment.txt/g' ${OUTDIR}/rootfs/home/finder-test.sh
    cp autorun-qemu.sh  ${OUTDIR}/rootfs/home/
fi
ls ${OUTDIR}/rootfs/home
cd ${OUTDIR}/rootfs
# TODO: Chown the root directory
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -o --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
