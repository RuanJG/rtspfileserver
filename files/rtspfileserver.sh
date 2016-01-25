#!/bin/sh 
net=eth0
nip='192.168.2.1'

camera_dev=$1
port=$2

startCamera(){
	if [ -z $camera_dev ] || [ ! -e $camera_dev ];then
		echo no find $camera_dev
		exit;
	else
		if [ -z $port ];then
			echo use port 8554 default
			port=8554
		fi
		echo start camera $camera_dev in $nip:$port
		/bin/rtspfileserver 320 240 $nip $port $camera_dev
	fi
}


while [ 0 ]
do
	ip=$(ifconfig $net  | grep 'inet addr')
	ip=${ip#*'inet addr:'}
	ip=${ip%' Bcast:'*}
	ip=${ip%' '}
	if [ -z $ip ];then
		echo no ip
	else
		#setdate
		echo $ip
		if [ "$nip" = "dhcp" ] || [ "$ip" = "$nip" ];then
			startCamera
		else
			echo can not connect ip $nip
		fi
	fi
	sleep 3
done
