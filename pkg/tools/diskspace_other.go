//go:build !linux && !darwin && !freebsd && !windows

package tools

import "errors"

// diskSpace is not implemented on this platform; diagnostics reports why.
func diskSpace(string) (free, total uint64, err error) {
	return 0, 0, errors.New("free disk is not reported on this platform")
}
