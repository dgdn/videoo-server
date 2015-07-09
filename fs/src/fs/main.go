package main

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"time"
)

func handler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Hi there, I love %s!", r.URL.Path[1:])
}
func upload(w http.ResponseWriter, r *http.Request) {
	r.ParseMultipartForm(32 << 20)
	file, handler, err := r.FormFile("uploadfile")
	fmt.Println("file field %v", handler.Filename)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer file.Close()
	ext := filepath.Ext(handler.Filename)
	fileName := fmt.Sprint(time.Now().UnixNano()) + ext
	f, err := os.OpenFile("./content/"+fileName, os.O_WRONLY|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer f.Close()
	io.Copy(f, file)
	fmt.Printf("upload success with file name", fileName)
	fmt.Fprintf(w, fmt.Sprintf(`{"files":[{"name":"%s"}]}`, fileName))
}

func innerUpload(w http.ResponseWriter, r *http.Request) {
	fmt.Println("enter upload method")
	r.ParseMultipartForm(32 << 20)
	file, handler, err := r.FormFile("uploadfile")
	if err != nil {
		fmt.Println(err)
		return
	}
	defer file.Close()
	fmt.Fprintf(w, "%v", handler.Header)
	f, err := os.OpenFile("./content/encoded/"+handler.Filename, os.O_WRONLY|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println(err)
		return
	}
	fmt.Println("begin to stream file")
	defer f.Close()
	io.Copy(f, file)
}

func main() {
	fmt.Println("fs server started")
	http.Handle("/", http.FileServer(http.Dir("content")))
	http.HandleFunc("/upload", upload)
	http.HandleFunc("/inner-upload", innerUpload)
	http.ListenAndServe(":5000", nil)
}
