#!/bin/bash

# Copyright (C) 2018 SUSE
# Author: Libor Pechacek
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

# Test Case 9: Exercise stack checking
# Try patching a function which is positively on stack of some process and
# check that KLP waits until the process leaves kernel space.

set -e
. $(dirname $0)/klp_tc_functions.sh
klp_tc_init "Test Case 9: Exercise stack checking"

klp_tc_milestone "Compiling sleep test module"
PATCH_DIR="/tmp/live-patch/tc_9"
SLEEP_MOD_NAME="klp_tc_9_sleep_test_mod"
klp_compile_patch_module "$PATCH_DIR" "$SLEEP_MOD_NAME".c

klp_tc_milestone "Inserting sleep test module"
insmod "$PATCH_DIR/$SLEEP_MOD_NAME".ko
if [ ! -e /sys/module/"$SLEEP_MOD_NAME" ]; then
   klp_tc_abort "don't see $SLEEP_MOD_NAME in modules sys directory"
fi
push_recovery_fn "rmmod $SLEEP_MOD_NAME"

klp_tc_milestone "Compiling kernel live patch"
PATCH_MOD_NAME="klp_tc_9_live_patch_vfs_read"
klp_compile_patch_module "$PATCH_DIR" "$PATCH_MOD_NAME".c

klp_tc_milestone "Make a process sleep for 10s"
cat /sys/kernel/debug/klp_tc9/sleep_10s &
push_recovery_fn "kill $!"
BLOCKED_SINCE=$(cut -d. -f1 /proc/uptime)

klp_tc_milestone "Inserting vfs_read() patch"
insmod "$PATCH_DIR/$PATCH_MOD_NAME".ko
if [ ! -e /sys/kernel/livepatch/"$PATCH_MOD_NAME" ]; then
   klp_tc_abort "don't see $PATCH_MOD_NAME in live patch sys directory"
fi
register_mod_for_unload "$PATCH_MOD_NAME"

klp_tc_milestone "Wait for completion"
if ! klp_wait_complete 71; then  # waiting 61+10 seconds
    klp_dump_blocking_processes
    klp_tc_abort "patching didn't finish in time"
fi
BLOCKED_UNTIL=$(cut -d. -f1 /proc/uptime)

BLOCKED_SECS=$(($BLOCKED_UNTIL - $BLOCKED_SINCE))
if [ "$BLOCKED_SECS" -lt 10 ]; then
   klp_tc_abort "patching finished too early ($BLOCKED_SECS s)"
fi

klp_tc_milestone "Removing sleep test module"
rmmod "$SLEEP_MOD_NAME"

klp_tc_exit
