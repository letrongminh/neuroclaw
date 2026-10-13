package tools

import (
	"context"
	"encoding/json"
	"runtime"
	"testing"
)

func TestDiagnosticsTool_ReportsProcessAndRuntimeInfo(t *testing.T) {
	tool := NewDiagnosticsTool(t.TempDir(), func() map[string]any {
		return map[string]any{"channels_enabled": []string{"telegram"}}
	})
	if tool.Name() != "get_diagnostics" {
		t.Fatalf("Name() = %q", tool.Name())
	}

	result := tool.Execute(context.Background(), nil)
	if result.IsError || !result.Silent {
		t.Fatalf("Execute() = %+v, want a silent, successful result", result)
	}
	var report map[string]any
	if err := json.Unmarshal([]byte(result.ForLLM), &report); err != nil {
		t.Fatalf("result is not JSON: %v\n%s", err, result.ForLLM)
	}
	for _, key := range []string{
		"version", "os", "arch", "uptime_sec", "memory", "goroutines", "workspace_disk", "channels_enabled",
	} {
		if _, ok := report[key]; !ok {
			t.Errorf("report misses %q", key)
		}
	}
	if report["os"] != runtime.GOOS {
		t.Errorf("os = %v, want %s", report["os"], runtime.GOOS)
	}
	disk, _ := report["workspace_disk"].(map[string]any)
	if free, ok := disk["free_bytes"].(float64); !ok || free <= 0 {
		t.Errorf("workspace_disk = %v, want free_bytes > 0 on the test host", disk)
	}
}

func TestDiagnosticsTool_DiskErrorIsReportedNotFatal(t *testing.T) {
	tool := NewDiagnosticsTool("", nil)
	disk, _ := tool.Report()["workspace_disk"].(map[string]any)
	if _, ok := disk["error"]; !ok {
		t.Fatalf("workspace_disk = %v, want an error entry for an empty path", disk)
	}
}
