package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"mime/multipart"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"strings"
)

func genOutFileName(rawSrcURL string) (string, error) {
	srcURL, err := url.Parse(rawSrcURL)
	if err != nil {
		return "", fmt.Errorf("url format err")
	}
	paths := strings.Split(srcURL.Path, "/")
	if len(paths) < 1 {
		return "", fmt.Errorf("url has no path")
	}
	rname := strings.Split(paths[len(paths)-1], ".")
	if len(rname) < 1 {
		return "", fmt.Errorf("file name format wrong in url")
	}
	return rname[0] + "_enc.flv", nil
}

var lastFileName string

func handler2(w http.ResponseWriter, r *http.Request) {
	fileName := r.FormValue("filename")
	encodeHost := os.Getenv("ENCODE_PORT_6000_TCP_ADDR")
	fsHost := os.Getenv("FS_PORT_5000_TCP_ADDR")
	srcUrl := fmt.Sprintf("http://%s:5000/%s", fsHost, fileName)
	resp, err := http.Get(fmt.Sprintf("http://%s:6000/encode?src_url=%s", encodeHost, srcUrl))
	if err != nil {
		fmt.Println(err)
		fmt.Fprint(w, "encode fail")
		return
	}
	if resp.StatusCode != 200 {
		fmt.Fprint(w, "encode fail")
		return
	}
	lastFileName = fileName
	fmt.Fprint(w, "success")
}

func getLastActionInfoHandler(w http.ResponseWriter, r *http.Request) {

	v, _ := json.Marshal(map[string]string{
		"fileName": lastFileName,
		"host":     os.Getenv("APP_IP"),
	})
	fmt.Fprintf(w, string(v))
}
func handler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Hi there, I love %s!", r.URL.Path[1:])
	rawSrcURL := r.FormValue("src_url")
	output, err := genOutFileName(rawSrcURL)
	if err != nil {
		log.Println(err)
	}
	fmt.Printf("output file name is %v", output)
	cmd := exec.Command("ffmpeg", "-i", rawSrcURL, "data/"+output)
	//cmd.Stdin = strings.NewReader("some input")
	var out bytes.Buffer
	cmd.Stdout = &out
	err = cmd.Run()
	if err != nil {
		log.Println(err)
	}
	err = postFile(output, fmt.Sprintf("http://%s:5000/inner-upload", os.Getenv("FS_PORT_5000_TCP_ADDR")))
	if err != nil {
		log.Println("upload file fail %v", err)
	}
	fmt.Printf("in all caps: %q\n", out.String())
}

func postFile(filename string, targetUrl string) error {
	bodyBuf := &bytes.Buffer{}
	bodyWriter := multipart.NewWriter(bodyBuf)

	// this step is very important
	fileWriter, err := bodyWriter.CreateFormFile("uploadfile", filename)
	if err != nil {
		fmt.Println("error writing to buffer")
		return err
	}

	// open file handle
	fh, err := os.Open("data/" + filename)
	if err != nil {
		fmt.Println("error opening file")
		return err
	}

	//iocopy
	_, err = io.Copy(fileWriter, fh)
	if err != nil {
		return err
	}

	contentType := bodyWriter.FormDataContentType()
	bodyWriter.Close()

	log.Println("target url %v", targetUrl)

	resp, err := http.Post(targetUrl, contentType, bodyBuf)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	resp_body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return err
	}
	fmt.Println(resp.Status)
	fmt.Println(string(resp_body))
	return nil
}

func main() {
	http.Handle("/", http.FileServer(http.Dir("www")))
	http.HandleFunc("/handle", handler2)
	http.HandleFunc("/last", getLastActionInfoHandler)
	http.ListenAndServe(":9000", nil)
	fmt.Println("app server started")
}
