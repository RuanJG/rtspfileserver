#!/bin/sh 
net='wlan0'
while [ 0 ]
do
	ip=$(ifconfig $net | grep 'inet addr')
	ip=${ip#*'inet addr:'}
	ip=${ip%' Bcast:'*}
	if [ "$ip"'x' = 'x' ];then
		echo can not find ip in br-lan
	else
        	echo "start rtspfileserver, in $ip"
		/bin/rtspfileserver 320 240
	fi
	sleep 3
done
