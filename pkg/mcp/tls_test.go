package mcp

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/tls"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"math/big"
	"net"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/letrongminh/neuroclaw/pkg/config"
)

type testPKI struct {
	dir     string
	caPool  *x509.CertPool
	server  tls.Certificate
	ca      string
	cert    string
	key     string
	rogueCa string
}

func issue(
	t *testing.T,
	template, parent *x509.Certificate,
	parentKey *ecdsa.PrivateKey,
) (*x509.Certificate, *ecdsa.PrivateKey, []byte, []byte) {
	t.Helper()
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		t.Fatal(err)
	}
	if parent == nil {
		parent, parentKey = template, key
	}
	der, err := x509.CreateCertificate(rand.Reader, template, parent, &key.PublicKey, parentKey)
	if err != nil {
		t.Fatal(err)
	}
	cert, err := x509.ParseCertificate(der)
	if err != nil {
		t.Fatal(err)
	}
	keyDER, err := x509.MarshalECPrivateKey(key)
	if err != nil {
		t.Fatal(err)
	}
	return cert, key,
		pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: der}),
		pem.EncodeToMemory(&pem.Block{Type: "EC PRIVATE KEY", Bytes: keyDER})
}

func newTestPKI(t *testing.T) testPKI {
	t.Helper()
	dir := t.TempDir()
	now := time.Now()
	caTemplate := &x509.Certificate{
		SerialNumber: big.NewInt(1), Subject: pkix.Name{CommonName: "test CA"},
		NotBefore: now.Add(-time.Hour), NotAfter: now.Add(time.Hour),
		IsCA: true, BasicConstraintsValid: true, KeyUsage: x509.KeyUsageCertSign,
	}
	ca, caKey, caPEM, _ := issue(t, caTemplate, nil, nil)
	_, _, roguePEM, _ := issue(t, &x509.Certificate{ //nolint:dogsled // only the PEM of a rogue CA is needed
		SerialNumber: big.NewInt(9), Subject: pkix.Name{CommonName: "rogue CA"},
		NotBefore: now.Add(-time.Hour), NotAfter: now.Add(time.Hour),
		IsCA: true, BasicConstraintsValid: true, KeyUsage: x509.KeyUsageCertSign,
	}, nil, nil)
	_, _, serverPEM, serverKey := issue(t, &x509.Certificate{
		SerialNumber: big.NewInt(2), Subject: pkix.Name{CommonName: "localhost"},
		NotBefore: now.Add(-time.Hour), NotAfter: now.Add(time.Hour),
		ExtKeyUsage: []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		IPAddresses: []net.IP{net.ParseIP("127.0.0.1")}, DNSNames: []string{"localhost"},
	}, ca, caKey)
	_, _, clientPEM, clientKey := issue(t, &x509.Certificate{
		SerialNumber: big.NewInt(3), Subject: pkix.Name{CommonName: "brain"},
		NotBefore: now.Add(-time.Hour), NotAfter: now.Add(time.Hour),
		ExtKeyUsage: []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
	}, ca, caKey)
	server, err := tls.X509KeyPair(serverPEM, serverKey)
	if err != nil {
		t.Fatal(err)
	}
	write := func(name string, data []byte) string {
		path := filepath.Join(dir, name)
		if err := os.WriteFile(path, data, 0o600); err != nil {
			t.Fatal(err)
		}
		return path
	}
	pool := x509.NewCertPool()
	pool.AddCert(ca)
	return testPKI{
		dir: dir, caPool: pool, server: server,
		ca: write("ca.pem", caPEM), cert: write("brain.pem", clientPEM),
		key: write("brain.key", clientKey), rogueCa: write("rogue.pem", roguePEM),
	}
}

// a server like `neuroedge mcp serve --http`: TLS 1.3, a client certificate signed by its CA required
func mutualTLSServer(t *testing.T, pki testPKI) *httptest.Server {
	t.Helper()
	server := httptest.NewUnstartedServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte(r.TLS.PeerCertificates[0].Subject.CommonName))
	}))
	server.TLS = &tls.Config{
		Certificates: []tls.Certificate{pki.server},
		ClientCAs:    pki.caPool,
		ClientAuth:   tls.RequireAndVerifyClientCert,
		MinVersion:   tls.VersionTLS13,
	}
	server.StartTLS()
	t.Cleanup(server.Close)
	return server
}

func get(t *testing.T, url string, cfg *config.MCPTLSConfig) (string, error) {
	t.Helper()
	transport, err := mcpHTTPTransport(url, cfg)
	if err != nil {
		return "", err
	}
	response, err := (&http.Client{Transport: transport, Timeout: 5 * time.Second}).Get(url)
	if err != nil {
		return "", err
	}
	defer response.Body.Close()
	body := make([]byte, 64)
	n, _ := response.Body.Read(body)
	return string(body[:n]), nil
}

func TestMCPTLS_TheBrainReachesABoxThatRequiresItsCertificate(t *testing.T) {
	pki := newTestPKI(t)
	server := mutualTLSServer(t, pki)
	who, err := get(t, server.URL, &config.MCPTLSConfig{CAFile: pki.ca, CertFile: pki.cert, KeyFile: pki.key})
	if err != nil {
		t.Fatalf("mutual TLS failed: %v", err)
	}
	if who != "brain" {
		t.Fatalf("server saw %q, want the brain's certificate", who)
	}
}

func TestMCPTLS_WithoutACertificateTheBoxRefuses(t *testing.T) {
	pki := newTestPKI(t)
	server := mutualTLSServer(t, pki)
	if _, err := get(t, server.URL, &config.MCPTLSConfig{CAFile: pki.ca}); err == nil {
		t.Fatal("a client without a certificate got through")
	}
}

func TestMCPTLS_ABoxSignedByAnotherCAIsNotTrusted(t *testing.T) {
	pki := newTestPKI(t)
	server := mutualTLSServer(t, pki)
	_, err := get(t, server.URL, &config.MCPTLSConfig{CAFile: pki.rogueCa, CertFile: pki.cert, KeyFile: pki.key})
	if err == nil || !strings.Contains(err.Error(), "certificate") {
		t.Fatalf("trusted a box outside ca_file: %v", err)
	}
}

func TestMCPTLS_ConfigurationMistakesAreRefusedBeforeConnecting(t *testing.T) {
	pki := newTestPKI(t)
	cases := map[string]struct {
		url string
		cfg *config.MCPTLSConfig
	}{
		"plain http":          {"http://127.0.0.1:1/mcp", &config.MCPTLSConfig{CAFile: pki.ca}},
		"certificate, no key": {"https://127.0.0.1:1/mcp", &config.MCPTLSConfig{CertFile: pki.cert}},
		"missing CA file": {
			"https://127.0.0.1:1/mcp",
			&config.MCPTLSConfig{CAFile: filepath.Join(pki.dir, "absent.pem")},
		},
		"CA file that is a key": {"https://127.0.0.1:1/mcp", &config.MCPTLSConfig{CAFile: pki.key}},
	}
	for label, c := range cases {
		if _, err := mcpHTTPTransport(c.url, c.cfg); err == nil {
			t.Errorf("%s: accepted", label)
		}
	}
}

func TestMCPTLS_NoTLSKeepsTheDefaultTransport(t *testing.T) {
	transport, err := mcpHTTPTransport("http://127.0.0.1:1/mcp", nil)
	if err != nil || transport != http.DefaultTransport {
		t.Fatalf("got %v, %v", transport, err)
	}
}
