#!/bin/bash

if [[ ! -f scripts/kconfig/cfconfig ]]; then
    make build_cfconfig
fi
mkdir -p new-configs
NREPD=ex #non-repro
iter=1
for conf in $(find $NREPD -maxdepth 1 -mindepth 1 | sort) ;
do
    cp $conf .config
    diag=""
    for opt in "MODULE_SIG_SHA1" "GCOV_PROFILE_FTRACE" \
	       "GCOV_PROFILE_ALL" "DEBUG_INFO_SPLIT"   \
	       "DEBUG_INFO_REDUCED" ; do
	if [[ $(grep "${opt}=y" .config) ]] ; then
	    diag="${diag} $opt n"
	fi
    done
    num=$(basename $conf | cut -d_ -f1)
    echo "$num,$diag" >> configs-diag
    export SRCARCH=x86 LD=$(which ld) CC=$(which gcc) srctree=$(pwd) ; scripts/kconfig/cfconfig Kconfig ${diag}
    for new in $(find . -maxdepth 1 -mindepth 1  -name ".config.*"); do
	mv $new new-configs/config${iter}
	iter=$[$iter+1]
    done
done
