package mcp

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"net/http"
	"net/url"
	"os"

	"github.com/letrongminh/neuroclaw/pkg/config"
)

// mcpHTTPTransport is the base transport for an HTTP MCP server: the default one, or,
// with cfg.TLS, a clone that presents a client certificate over TLS 1.3 and trusts only
// the given CA. NeuroClaw: a NeuroEdge home box (`mcp serve --http`) refuses any client
// without a certificate it signed, so the brain needs no mTLS bridge beside it.
func mcpHTTPTransport(serverURL string, cfg *config.MCPTLSConfig) (http.RoundTripper, error) {
	if cfg == nil {
		return http.DefaultTransport, nil
	}
	parsed, err := url.Parse(serverURL)
	if err != nil || parsed.Scheme != "https" {
		return nil, fmt.Errorf("tls is set but url %q is not https", serverURL)
	}
	if (cfg.CertFile == "") != (cfg.KeyFile == "") {
		return nil, fmt.Errorf("tls needs both cert_file and key_file, or neither")
	}
	tlsConfig := &tls.Config{MinVersion: tls.VersionTLS13, ServerName: cfg.ServerName}
	if cfg.CAFile != "" {
		pem, err := os.ReadFile(expandHomeCommandPath(cfg.CAFile))
		if err != nil {
			return nil, fmt.Errorf("tls ca_file: %w", err)
		}
		roots := x509.NewCertPool()
		if !roots.AppendCertsFromPEM(pem) {
			return nil, fmt.Errorf("tls ca_file %q holds no PEM certificate", cfg.CAFile)
		}
		tlsConfig.RootCAs = roots
	}
	if cfg.CertFile != "" {
		pair, err := tls.LoadX509KeyPair(
			expandHomeCommandPath(cfg.CertFile), expandHomeCommandPath(cfg.KeyFile))
		if err != nil {
			return nil, fmt.Errorf("tls cert_file/key_file: %w", err)
		}
		tlsConfig.Certificates = []tls.Certificate{pair}
	}
	base, ok := http.DefaultTransport.(*http.Transport)
	if !ok {
		return nil, fmt.Errorf("http.DefaultTransport is not an *http.Transport")
	}
	transport := base.Clone()
	transport.TLSClientConfig = tlsConfig
	return transport, nil
}
