#!/bin/sh
# Subset of bs_funcs.sh, just enough to provide the function bs_stamp_debian_changelog()
set -e
set -x

#---- begin verbatim from bs_funcs.sh ----

# Print a message and terminate with nonzero status.
bs_abort() {
    echo fatal error: $*
    exit 1
}

# Echo the version number of this product as given by git
# This works for projects that name branches like kernel.org, Wine, or Node do
bs_get_version_git() {
    # git describe --long's output looks like
    # name-COUNT-CHECKSUM
    # or, if at a tag,
    # name
    d1=`git describe --tags --long`
    # Strip off -CHECKSUM suffix, if any
    d2=`echo $d1 | sed 's/-[a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9]$//'`
    # Strip off -COUNT suffix, if any
    d3=`echo $d2 | sed 's/-[0-9]*$//'`
    # Remove non-numeric prefix (e.g. rel- or debian/), if any
    d4=`echo $d3 | sed 's/^[^0-9]*//'`
    # Remove non-numeric suffix (e.g. -mz-gouda), if any
    d5=`echo $d4 | sed 's/-[^0-9]*$//'`
    case "$d5" in
    "") bs_abort "can't parse version number from git describe --long's output $d1";;
    esac
# gaaah.  bs_funcs.sh didn't expect a 2 in the component name.
d6=`echo $d5 | sed 's/-p2md//'`
    echo $d6
}

# Echo the change number since the start of this branch as given by git
bs_get_changenum_git() {
    # git describe --long's output looks like
    # name-COUNT-CHECKSUM
    # First strip off the checksum field, then the name.
    if ! d1=`git describe --long --tags 2> /dev/null`
    then
        # No releases!  Just count changes since epoch.
        git log --oneline | wc -l | sed 's/^[[:space:]]*//'
        return 0
    fi
    d2=`echo $d1 | sed 's/-[a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9]$//'`
    d3=`echo $d2 | sed 's/^.*-//'`
    case "$d3" in
    "") bs_abort "can't parse change number from git describe --long's output $d1";;
    esac
    echo $d3
}

bs_stamp_debian_changelog() {
    distro_codename=`lsb_release -c -s`

    case "$version" in
    [0-9]*) ;;
    *) bs_abort "bs_stamp_changelog: bad version '$version'";;
    esac
    case "$version_patchnum" in
    [0-9]*) ;;
    *) bs_abort "bs_stamp_changelog: bad version_patchnum '$version_patchnum'";;
    esac

    # If patchnum is empty or zero, don't append it as suffix
    case "$version_patchnum" in
    ""|0) suffix="";;
    *) suffix="-$version_patchnum";;
    esac

    sed -i "1s/(.*/($version$suffix) $distro_codename; urgency=low/" debian/changelog
}

#---- end verbatim from bs_funcs.sh ----

#---- begin mostly verbatim from bs_funcs.sh ----
version=`bs_get_version_git`
version_patchnum=`bs_get_changenum_git`

#---- end mostly verbatim from bs_funcs.sh ----

bs_stamp_major_version_after_x() {
    # Assume that, on any line containing the word oblong,
    # any number preceded by an x is the major version number;
    # replace it with the new major version number with extreme prejudice.
    # Caution: don't change any x's not followed by numbers (e.g. foo-gs3.16x).
    # FIXME: what about lines with x86 in them?
    # Note: on mac, -i must be followed by a suffix.
    # FIXME: find better way to figure out which files need this munging.
    sed -i.vbak \
        -e "/oblong/s/x[0-9][0-9]*/x$version_major/g" \
        debian/control debian/rules debian/changelog
    mv -f debian/*.vbak /tmp

    # Do the same thing for filenames in debian directory.
    for from in debian/oblong*x[0-9]*
    do
        to=`echo $from | sed "s/x[0-9][0-9]*/x$version_major/"`
        if test $from != $to
        then
            # When playing for keeps, do 'export GITCMD=git'
            $GITCMD mv $from $to
        fi
    done
}

case "$1" in
[0-9]*)
    version=$1
    version_patchnum=0
    ;;
"")
    ;;
*)
    set +x
    bs_abort "Usage: sh set-version.sh [X.Y]\n\
If run with no arguments, gets the version number from 'git describe'.\n\
Stamps the version number into debian/changelog\n\
and stamps the major version number on top of any number after\n\
'oblong...x' in filenames or contents.\n\
"
    ;;
esac

# split $version into major.minor.micro.nano-suffix
version_prefix=${version%%-*}     # remove longest suffix that starts with -
version_suffix=
case $version in
*-*) version_suffix=${version##*-} ;;    # remove longest prefix that ends with -
esac
version_major=`echo $version_prefix | cut -d. -f1`
version_minor=`echo $version_prefix | cut -d. -f2`
version_micro=`echo $version_prefix | cut -d. -f3`
version_nano=`echo $version_prefix | cut -d. -f4`
test "$version_micro" = "" && version_micro=0
if test $version = $version_major.$version_minor.$version_micro.$version_nano
then
    # wow, a four-component version number.  Careful, something might break.
    :
elif test $version != $version_major.$version_minor.$version_micro && test $version != $version_major.$version_minor
then
    bs_abort "Failed to parse $version into major.minor.micro"
fi

bs_stamp_debian_changelog
bs_stamp_major_version_after_x
