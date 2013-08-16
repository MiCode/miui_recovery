#!/sbin/sh
export PATH=/sbin

nohup /sbin/stock > /dev/null &
recovery_pid="$!"
sleep 2
kill -STOP "$recovery_pid"
/sbin/recovery

