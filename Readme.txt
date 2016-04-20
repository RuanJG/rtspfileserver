rtspfileserver: 提供usb摄像头rtspe服务程序

编译安装：
cd src/
make
make install

用法：
./rtspfileserver 320 240 $ip $port $camera_dev


files/rtspfileserverd 系统服务文件调用rtspfileserver.sh
files/rtspfileserver.sh 调用rtspfileserver程序

注意，在rtspfileserver.sh里程序邦定的IP是eth0的IP，所以如果系统的IP有变化，这里要修改，
