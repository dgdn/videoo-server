run(){
	docker build -t fs .
	docker run --rm -p 5000:80 fs
}

IP="$(docker-machine ip)"

upload(){
	curl -i -X POST -H "Content-Type: multipart/form-data" \
	-F "uploadfile=@test-data/o.flv" http://$IP:5000/inner-upload
}

upload2(){
	curl -i -X POST -H "Content-Type: multipart/form-data" \
	-F "uploadfile=@test-data/o.flv" http://$IP:5000/upload
}

case $1 in
	"upload")
		upload
		;;
	"upload2")
		upload2
		;;
	"run")
		run
		;;
	*)
	run
	;;	
esac
