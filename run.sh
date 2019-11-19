#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>

TESTS="
klp_tc_3.sh|Patch under pressure
klp_tc_5.sh|Test live kernel patching in quick succession
klp_tc_6.sh|Patch while CPUs are busy
klp_tc_7.sh|Patch in low memory condition
klp_tc_8.sh|Patch with replace-all
klp_tc_10.sh|Patch caller of graph traced callee
klp_tc_11.sh|Patch function sleeping in a fault
klp_tc_12.sh|Patch caller of kretprobed callee
klp_tc_13.sh|Patch traced function
klp_tc_14.sh|Trace patched function
klp_tc_15.sh|Patch graph-traced function
klp_tc_16.sh|Graph-trace patched function
klp_tc_17.sh|Check that patching a kprobed function fails
"

bats=$(which bats 2>/dev/null)
script=$(mktemp)

if [ "$bats" ]; then
    echo "#!$bats" > $script
else
    cat > $script <<EOF
#!/bin/sh
ret=0
EOF
fi

IFS="
"
for i in $TESTS; do
    file=$(echo $i | cut -d'|' -f1)
    desc=$(echo $i | cut -d'|' -f2)

    if [ "$bats" ]; then
        cat >> $script <<EOF
@test "$desc" {
    ./$file
}
EOF
    else
        cat >> $script <<EOF
echo "== $desc =="
./$file || ret=1
echo
EOF
    fi
done

[ ! "$bats" ] && echo 'exit $ret' >> $script

chmod 755 $script

$script
exit $?
