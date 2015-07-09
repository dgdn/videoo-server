#!/bin/bash
auth(){
	rm -rf plugins/auth/sampleAuth64 
	docker run --rm -v `pwd`/plugins:/plugins -w /plugins/auth ams make
	cp plugins/auth/sampleAuth64/AuthModule.so modules/auth
}

dp(){
	auth
	rm logs/auth*
	docker-compose up -d
}

run(){
	auth
	docker run --rm -p 80:80 -p 1111:1111 -p 1935:1935 -w /opt/adobe/ams -v `pwd`/modules/auth:/opt/adobe/ams/modules/auth ams sh -c './amsmgr server ams start && ./amsmgr adminserver start && mkdir -p logs/authMessage.00.log || tail -f logs/authMessage.00.log'
}

case $1 in 
	"auth")
	auth
	;;
	"dp")
	dp
	;;
	"run")
	run
	;;
esac
