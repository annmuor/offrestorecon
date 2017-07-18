#!/bin/sh
/usr/sbin/selinuxenabled && exit 1
test -f /.noautorelabel && exit 1
exit 0
