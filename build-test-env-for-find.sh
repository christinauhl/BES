#!/bin/bash --norc
#
# Bernd Petrovitsch <bernd.petrovitsch@technikum-wien.at>
#
# Macht eine Directoryhierarchie um `find` zu testen
#
# @todo Use XFS to trigger d_type of struct dirent to be DT_UNKNOWN
# and thus cause all implementations to fail that do not use the
# st_mode member of the struct stat. - To create the appropriate XFS
# filesystem following needs to be done (setting ftype to 0 is explicitly
# required on my machine (ubuntu 16.10)):
# sudo mkfs.xfs -f -n ftype=0 -m crc=0 /dev/sdd2
#


set -ue	# error out if we use uninitialized variables and on failed command
#set -vx # for debugging

#echo "Please customize the variables in the script!"; exit 0
#and comment then the previous line out!

if [ "$(id -u)" -ne 0 ]; then
    echo "You must be root run this script!"
    exit 1
fi

# customizing
readonly TOPDIR="/var/tmp/test-find" # where to install that
readonly NUM_USERNAME="160"
readonly NUM_UID="150"
readonly NUM_OTHERUSERNAME="karl"
readonly NOT_USED_UID="999999"
readonly LONG_USERNAME="d836154a1ba14015bff78dad09f7dd82b41cd39c2a1347598215ad0622e87d03d2cc91334a4e4d8e9a5b4f35a8650d2cf5a6711d86de4b1bb227eb8ccb58bcf9197878c566d84050ae7f1aca5c6b97ab"
readonly SPECIAL_USERNAME="x%sx%px%n"


# make sure the necessary users and groups according to create-accounts.pl exist!

make_funny_files() {
    : ein File, das fuer den Parametercheck genutzt wird  > so-nicht
    chown "${NOT_USED_UID}" "so-nicht" # und ein nicht existenter user ....
    : noch ein File, das fuer den Parametercheck genutzt wird > so
    : usercheck > "${NUM_OTHERUSERNAME}"
    chown "${NUM_OTHERUSERNAME}" "${NUM_OTHERUSERNAME}" # und ein existenter user ....

    # create a (text) file
    echo "Hello world" > "plain-file"
    echo "Hello world" > ".hidden-file"
    
# files with glob chars (and other ugly ones)
    : > "*"
    : > "?"
    : > "["
    : > "]"
    : > "file\\with\\escape\\character"
# files with names like the options
    for file in "-type" "-name" "-path" "-ls" "-print" "-user" "-group" "-nouser" "-nogroup"; do
        : > "./$file"
    done
# files with names looking like format strings, especially pointer-like ones
    for ch in d u p s x; do
        : > "%$ch"
        : > "%*$ch"
        : > "%.*$ch"
        : > "%*.*$ch"
    done
    : > "%n"

    # create directories
    mkdir "empty" "not-empty" ".empty-hidden" ".not-empty-hidden"
    # and some contents
    echo "Hello world again" > "not-empty/another-plain-file"
    echo "Hello world again" > "not-empty/.another-hidden-file"
    echo "Hello world again" > ".not-empty-hidden/another-plain-file"
    echo "Hello world again" > ".not-empty-hidden/.another-hidden-file"
    chown "${LONG_USERNAME}:${LONG_USERNAME}" "not-empty/another-plain-file" ".not-empty-hidden/another-plain-file"
    chown "${SPECIAL_USERNAME}:${SPECIAL_USERNAME}" "not-empty/.another-hidden-file" ".not-empty-hidden/.another-hidden-file"

    # create a file (containing only zeroes) with holes in it (a.k.a. sparse file)
    dd of="not-empty/file-without-holes" if="/dev/zero" bs=1024 count=10
    cp --sparse=always "not-empty/file-without-holes" "not-empty/file-with-holes"
# create a file with a size that is not an integral multiple of 1024
    dd of="not-empty/file-with-size-not-divisible-by-1024" if="/dev/zero" bs=512 count=13

# create hard-link
    ln "plain-file" "linked-plain-file"
    ln "plain-file" "not-empty/linked-plain-file"
# create sym-links
    ln -s "this-should-not-exist" "dangling-sym-link"
    ln -s "plain-file" "working-sym-link"
# and a long chain of sym-links
    for i in {2..22}; do
        ln -s "sym-link-$((${i} - 1))" "sym-link-${i}"
    done
    ln -s "plain-file" "sym-link-1"

# block, char device and a fifo
    mkfifo "named-pipe"
    chmod u=s,go= "named-pipe"
    mknod "block-device" "b" 999 999
    chmod u=,g=s,o= "block-device"
    mknod "char-device"  "c" 998 998
    chmod ug=,o=t "char-device"
    mksock "socket"
    chmod u=t,go= "socket"

    make_long_link
    make_deep_directory "$(uuidgen)"
    make_deep_directory " "
}

make_long_link() {
# test with a long sym-link
    local LONG="$(uuidgen)"
    while ln -sf "${LONG}" "long-link" 2> /dev/null; do
#    for i in {1..112}; do
        LONG="${LONG}$(uuidgen)"       
    done
}

make_deep_directory() {
# und ein tiefes langes directory
    (
        set -e
        local name="$1"
        local pathname="$name"
        while [ "${#pathname}" -lt "4095" ]; do
            pathname="$pathname/$name"
        done
        mkdir -p "./$pathname"
    ) || : # ignore errors
}

make_very_deep_directory() {
# und ein tiefes langes directory
    (
        local uuid
        set -e
        local name="$1"
        local pathname="$name"
        for i in {1..2048}
          do
          pathname="$pathname/$name"
          mkdir "${name}"
          cd "${name}" 2> /dev/null
        done
    ) || : # ignore errors
}

mksock() {
    perl -e "use IO::Socket::UNIX; IO::Socket::UNIX->new(Type => SOCK_STREAM(), Local => '$1', Listen => 1);"
}

umask 000

#umount "${TOPDIR}/xfs" || :
#losetup -d "/dev/loop0" || :
rm -rf "${TOPDIR}"
mkdir -p "${TOPDIR}/full" "${TOPDIR}/simple" "${TOPDIR}/xfs"

# fixup the permissions
chmod -R go-w "${TOPDIR}"
chown "$(id -u):$(id -g)" "${TOPDIR}" "${TOPDIR}/full" "${TOPDIR}/simple"

###################
# generate a few simple test cases
#
cd "${TOPDIR}/simple"

make_funny_files

###################
# generate a few simple test cases into an xfs filesystem
#
#set -vx
#dd if=/dev/zero of="${TOPDIR}/xfs-data" count=1 bs=$((16 * 1024 * 1024))
#losetup "/dev/loop0" "${TOPDIR}/xfs-data"
#mkfs.xfs "${TOPDIR}/xfs-data"

#exit 0

###################
# build a quite large set of files and the like with lots of combinations
#
cd "${TOPDIR}/full"

make_funny_files

make_long_link
#make_very_deep_directory "$(uuidgen)"

declare -a filetypes=(b c d f l p s)
i=0
# and a few files for the various types and permissions
for perms in u={r,w,x,s,xs},go= u=,g={r,w,x,s,xs},o= ug=,o={r,w,x,t,xt}
  do
  case $(($i % 6)) in
      0) mknod "block-device-${perms}" b 47 11
         chmod "${perms}" "block-device-${perms}"
         ;;
      1) mknod "char-device-${perms}" c 08 15
         chmod "${perms}" "char-device-${perms}"
         ;;
      2) mkdir "directory-${perms}"
         chmod "${perms}" "directory-${perms}"
         ;;
      3) mkfifo "fifo-${perms}"
         chmod "${perms}" "fifo-${perms}"
         ;;
      4) : > "plain-file-${perms}"
         chmod "${perms}" "plain-file-${perms}"
         ln -sf "plain-file-${perms}" "sym-link-${perms}"
         chmod "${perms}" "sym-link-${perms}"
         ;;
      5) mksock "socket-${perms}"
         chmod "${perms}" "socket-${perms}"
         ;;
  esac
  i=$(( $i + 1 ))
done

# play permission games
for perms in u={r,-}{w,-}{x,-}{s,},g={r,-}{w,-}{x,-}{s,},o={r,-}{w,-}{x,-}{t,}; do
    mknod "block-device-${perms}" b 47 11
    mknod "char-device-${perms}" c 08 15
    mkdir "directory-${perms}"
    mkfifo "fifo-${perms}"
    : > "plain-file-${perms}"	# save fork(2)+exec(2) avoiding "touch"
    ln -sf "plain-file-${perms}" "sym-link-${perms}"
    mksock "socket-${perms}"
    chmod "${perms//-}" "block-device-${perms}" "char-device-${perms}" "directory-${perms}" "fifo-${perms}" "plain-file-${perms}" "sym-link-${perms}" "socket-${perms}"
done

# now create some more files
: > "test-${NUM_USERNAME}"	# save fork(2)+exec(2) avoiding "touch"
: > "test-${NUM_OTHERUSERNAME}"	# save fork(2)+exec(2) avoiding "touch"
chown "${NUM_USERNAME}:${NUM_UID}" "test-${NUM_USERNAME}"
chown "${NUM_OTHERUSERNAME}:${NUM_UID}" "test-${NUM_OTHERUSERNAME}"

exit 0

# Local Variables:
# sh-basic-offset: 4
# End:
