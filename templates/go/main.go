package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"net"
	"strconv"
)

// SocketClient contains all attributes to establish a socket connection
type SocketClient struct {
	Host   string
	Port   int
	conn   *net.TCPConn
	reader *bufio.Reader
}

func (sc *SocketClient) Write(command byte) error {
	_, err := sc.conn.Write([]byte{command})

	return err
}

// Open – Opens the socket connection 
func (sc *SocketClient) Open() error {
	url := *host + ":" + strconv.Itoa(sc.Port)
	log.Println("Connecting to " + url)
	add, err := net.ResolveTCPAddr("tcp", url)
	if err != nil {
		log.Panic("Unable to resolve host " + *host)
		return err
	}

	conn, err := net.DialTCP("tcp", nil, add)
	if err != nil {
		log.Println("Can't open connection")
		return err
	}

	sc.conn = conn
	sc.reader = bufio.NewReader(sc.conn)

	return nil
}

// Read - Reads as long as a square map is returned by the connected server
func (sc *SocketClient) Read() (string, error) {
	var readMap string
	readMap, err := sc.reader.ReadString('\n')

	if err != nil {
		return "", err
	}

	// subtract 1 for newline and 1 for first line
	mapDimension := len(readMap) - 2

	for i := 0; i < mapDimension; i++ {
		var newLine string
		newLine, err = sc.reader.ReadString('\n')
		if err != nil {
			break
		}
		readMap += newLine
	}

	return readMap, err
}

// Testcode

var host = flag.String("host", "localhost", "The gameserver to connect to")
var port = flag.Int("port", 63187, "The port for the socket connection")

func init() {
	flag.Parse()
}

var client SocketClient
var keepRunning = true

func main() {
	client = SocketClient{Host: *host, Port: *port}

	if err := client.Open(); err != nil {
		keepRunning = false
	}

	for keepRunning {
		gameMap, err := client.Read()
		fmt.Println(gameMap)

		var command string
		fmt.Println("Command(^v<>)")
		fmt.Scan(&command)

		err = client.Write(command[0])

		if err != nil {
			keepRunning = false
			fmt.Println(err)
		}
	}
}
