//go:build linux || darwin || freebsd

package tools

import (
	"errors"

	"golang.org/x/sys/unix"
)

// diskSpace returns free (available to this user) and total bytes of the
// filesystem holding path.
func diskSpace(path string) (free, total uint64, err error) {
	if path == "" {
		return 0, 0, errors.New("no workspace path")
	}
	var st unix.Statfs_t
	if err := unix.Statfs(path, &st); err != nil {
		return 0, 0, err
	}
	bsize := statfsUint(st.Bsize)
	return statfsUint(st.Bavail) * bsize, statfsUint(st.Blocks) * bsize, nil
}

// statfsUint widens a Statfs_t field, whose integer type differs per OS/arch.
func statfsUint[T ~int32 | ~int64 | ~uint32 | ~uint64](v T) uint64 {
	return uint64(v)
}
