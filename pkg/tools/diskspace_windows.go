//go:build windows

package tools

import (
	"errors"

	"golang.org/x/sys/windows"
)

// diskSpace returns free (available to this user) and total bytes of the
// volume holding path.
func diskSpace(path string) (free, total uint64, err error) {
	if path == "" {
		return 0, 0, errors.New("no workspace path")
	}
	p, err := windows.UTF16PtrFromString(path)
	if err != nil {
		return 0, 0, err
	}
	var totalFree uint64
	if err := windows.GetDiskFreeSpaceEx(p, &free, &total, &totalFree); err != nil {
		return 0, 0, err
	}
	return free, total, nil
}
