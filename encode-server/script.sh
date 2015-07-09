run(){
docker build -t encode-server .
docker run -it --rm -p 6000:80 -e "FS_HOST=$(docker-machine ip)" encode-server
}

IP=$(docker-machine ip)

test(){
	curl http://$IP:6000/encode?src_url=http://$IP:5000/1436341279846063586.flv
	curl http://$IP:5000/encoded/o_enc.flv
}

upload(){
	curl -i -X POST -H "Content-Type: multipart/form-data" \
	-F "data=@data/o.flv;filename=o.flv" http://$IP:5000/inner-upload
}

case $1 in
	"run")
		run
		;;
	"test")
		test
		;;
	"upload")
		upload
		;;
esac
