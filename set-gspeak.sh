#!/bin/sh
#
# Run this script before doing 'debuild' if you want to select a different
# version of g-speak.
#
# This script should also be run if the major version of the project changes;
# it will pick up the major version number from 'git describe' and
# adorn the package name with the suffix x$version_major
# This lets you choose which major version of the package you want at
# deploy time.

set -e
gspeak_version=$1
opt=$2
pkgname=`awk '/Source:/ {print $2};' < debian/control`
projname=`echo $pkgname | sed 's/-gs.*//'`

# If the following line fails, please install oblong's obs package.
# See https://github.com/Oblong/obs
. obs_funcs.sh

usage() {
    echo "Usage: $0 G_SPEAK_VERSION [--cef cefbranch]"
    echo "example: $0 3.20"
    echo "example: $0 3.21 --cef 2526"
}

version_major=`bs_get_major_version_git`

case "$gspeak_version" in
3.[0-9]*) ;;
*)  usage
    exit 1
    ;;
esac

G_SPEAK_HOME=/opt/oblong/g-speak$gspeak_version

cefbranch=`bs_yovo2cefversion $gspeak_version`
case "$opt" in
"--cef")
    cefbranch=cef$3
    ;;
"")
    ;;
*)
    usage
    exit 1
    ;;
esac
BITS=64
YOVERSION=`bs_yovo2yoversion $gspeak_version`
YOBUILD=/opt/oblong/deps-$BITS-$YOVERSION

set -x

sed -i.bak \
    -e "/^[^#]/s/g-speak[0-9].[0-9][0-9]*/g-speak$gspeak_version/g" \
    -e "s/gs[0-9].[0-9][0-9]*x/gs${gspeak_version}x/g" \
    -e "s/${projname}-gs[0-9.]*x[0-9]*/${projname}-gs${gspeak_version}x$version_major/g" \
    -e "s/-cef[0-9]*/-${cefbranch}/g" \
    -e "s,yobuild[0-9]*,yobuild$YOVERSION," \
    -e "s,YOBUILD=.*,YOBUILD=$YOBUILD," \
    debian/changelog debian/control debian/rules \
    #

echo "G_SPEAK_HOME=$G_SPEAK_HOME" > g-speak.dat

# No need to edit CMakeLists.txt, it grabs default value for G_SPEAK_HOME from g-speak.dat

# We only want first line of changelog to change
head -n 1 < debian/changelog > /tmp/tmp.$$
tail -n +2 < debian/changelog.bak >> /tmp/tmp.$$
mv /tmp/tmp.$$ debian/changelog

# Templates for the files that need renaming,
# with GG.HH for the g-speak major.minor number,
# and AA for this project's major number.
# This project's major number comes at the very end of the Debian package
# name (possibly before a -dbg suffix).
files="
${projname}-gsGG.HHxAA.install
"

# Update the version number in all instances of the above filenames in the debian directory
for file in $files
do
    from=`echo $file | sed 's/GG.HH/*[0-9].[0-9]*/;s/AA/[0-9]*/'`
    if ! from=`cd debian; ls $from 2>/dev/null`
    then
        # KLUDGE: allow project version to be absent if zero
        # This allows ratcheting upwards from zero
        from=`echo $file | sed 's/GG.HH/*[0-9].[0-9]*/;s/AA//'`
        if ! from=`cd debian; ls $from 2>/dev/null`
        then
            continue
        fi
    fi

    to=`echo $file | sed "s/GG.HH/$gspeak_version/;s/AA/$version_major/"`
    if test $from != $to
    then
        # Leave GITCMD empty when just building, set it to git when renaming in anger
        $GITCMD mv debian/$from debian/$to
    fi
done
