module example_client

replace github.com/chadbohannan/application-layer-network/go => ../

replace github.com/chadbohannan/application-layer-network/go/aln => ../../aln/

go 1.13

require (
	github.com/chadbohannan/application-layer-network/go/aln v0.0.0-00010101000000-000000000000
	github.com/google/uuid v1.3.0
)
