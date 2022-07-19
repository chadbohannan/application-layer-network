package aln

import (
	"crypto/tls"
	"fmt"
	"log"
	"os"
)

// implements ChannelHost
type SslChannelHost struct {
	name     string
	port     int
	keyFile  string
	certFile string
}

func NewSslChannelHost(name string, port int, keyFile, certFile string) *SslChannelHost {
	return &SslChannelHost{
		name:     name,
		port:     port,
		keyFile:  keyFile,
		certFile: certFile,
	}
}

// Listen for incoming connections. Blocks indefinetly.
func (host *SslChannelHost) Listen(onConnect func(Channel)) {
	bindAddress := fmt.Sprintf("%s:%d", host.name, host.port)

	cert, err := tls.LoadX509KeyPair(host.certFile, host.keyFile)
	if err != nil {
		log.Fatal(err)
	}
	config := &tls.Config{Certificates: []tls.Certificate{cert}}

	l, err := tls.Listen("tcp", bindAddress, config)
	if err != nil {
		fmt.Println("SslChannelHost:Listen err:", err.Error())
		os.Exit(-1)
	}
	defer l.Close()

	for {
		conn, err := l.Accept() // Listen for an incoming connection.
		if err != nil {
			fmt.Println("SslChannelHost:Accept err: ", err.Error())
			os.Exit(-1)
		}
		go onConnect(NewTCPChannel(conn))
	}
}
