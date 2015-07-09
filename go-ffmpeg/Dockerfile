FROM golang:1.4.2-wheezy
RUN git clone https://github.com/nachochip/ffmpeg
RUN	tar -xvzf ffmpeg/ffmpeg.tar.gz -C /usr/local/
RUN	cp ffmpeg/libc.conf /etc/ld.so.conf.d/

