FROM dgdn/ams
RUN yum -y install gcc-c++ nfs-utils nfs-utils-lib
WORKDIR /opt/adobe/ams
ADD ./test.mp4 /opt/adobe/ams/webroot/vod/
CMD ./amsmgr server ams start && ./amsmgr adminserver start && tail -f /opt/adobe/ams/logs/master.00.log
EXPOSE 80
EXPOSE 1111
EXPOSE 1935 
VOLUME /opt/adobe/ams/applications
VOLUME /opt/adobe/ams/conf
VOLUME /opt/adobe/ams/logs
