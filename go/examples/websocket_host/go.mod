module github.com/chadbohannan/application-layer-network/go/examples/websocket_host

go 1.21

replace github.com/chadbohannan/application-layer-network/go/aln => ../../aln

require (
	github.com/chadbohannan/application-layer-network/go/aln v0.0.0-00010101000000-000000000000
	github.com/gorilla/websocket v1.5.0
)
