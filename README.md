# videoo-server
live media server solution powered by adobe media server and docker
consist of 4 components  app server, file server for storage, encode server, ams.

this is the technical experiment for future reference

## dependency
docker and docker-compose

## simple running up step
1. change the ams/sample/videoPlayer/videoPaly.html content of ip addr to your own host local ip
2. run docker-compose -f docker-compose-local.yml up in the cli
3. set the browser to allow cors and open ams/sample/videoPaly/videoPlayer.html in the browser
4. click the upload button and select a video file to upload
5. wait a moment, the video can play in the flash player
6. use the address show in the page generated by the backend, you can play the video in iphone or android(see help in the client dir)
