##########################################################################
# File Name: mult_runer.sh
# Author: WangTeacher
# site:   www.daozy.net
# QQ群:   876662784
# Created Time: 2020年12月04日 星期五 06时10分00秒
#########################################################################
#!/bin/zsh
#PATH=$PATH:`pwd`
#export PATH
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`
export LD_LIBRARY_PATH

./parseipdb ipv4_cn.ipdb 0 20 > ip_info_0_20.txt &
./parseipdb ipv4_cn.ipdb 21 40 > ip_info_21_40.txt &
./parseipdb ipv4_cn.ipdb 41 60 > ip_info_41_60.txt &
./parseipdb ipv4_cn.ipdb 61 80 > ip_info_61_80.txt &
./parseipdb ipv4_cn.ipdb 81 100 > ip_info_81_100.txt &
./parseipdb ipv4_cn.ipdb 101 120 > ip_info_101_120.txt &
./parseipdb ipv4_cn.ipdb 121 140 > ip_info_121_140.txt &
./parseipdb ipv4_cn.ipdb 141 160 > ip_info_141_160.txt &
./parseipdb ipv4_cn.ipdb 161 180 > ip_info_162_180.txt &
./parseipdb ipv4_cn.ipdb 181 200 > ip_info_181_200.txt &
./parseipdb ipv4_cn.ipdb 201 220 > ip_info_201_220.txt &
./parseipdb ipv4_cn.ipdb 221 240 > ip_info_221_240.txt &
./parseipdb ipv4_cn.ipdb 241 255 > ip_info_240_255.txt &
