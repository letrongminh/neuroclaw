package tools

import (
	"context"
	"encoding/json"
	"maps"
	"runtime"
	"time"

	"github.com/letrongminh/neuroclaw/pkg/config"
)

// processStart approximates when this process started (package init).
var processStart = time.Now()

// DiagnosticsTool reports the health of this NeuroClaw process: build, host,
// uptime, memory, free disk of the workspace and, through runtimeInfo, the
// agent's own state (channels, model cooldowns, LLM budget, risk labels, last
// LLM error). It is read-only and the same tool name the ESP32 chip exposes.
//
// It must never report secrets: no API keys, tokens or URLs. runtimeInfo
// providers are responsible for what they add.
type DiagnosticsTool struct {
	workspace   string
	runtimeInfo func() map[string]any
	nowFunc     func() time.Time // for testing
}

// NewDiagnosticsTool creates a DiagnosticsTool. runtimeInfo may be nil.
func NewDiagnosticsTool(workspace string, runtimeInfo func() map[string]any) *DiagnosticsTool {
	return &DiagnosticsTool{workspace: workspace, runtimeInfo: runtimeInfo, nowFunc: time.Now}
}

func (t *DiagnosticsTool) Name() string {
	return "get_diagnostics"
}

func (t *DiagnosticsTool) Description() string {
	return "Report NeuroClaw's own health: version, OS/arch, uptime, memory, goroutines, " +
		"free disk of the workspace, enabled channels, model fallback/cooldown state, " +
		"LLM call budget, active risk labels and the last LLM error. Read-only; no secrets."
}

func (t *DiagnosticsTool) Parameters() map[string]any {
	return map[string]any{
		"type":       "object",
		"properties": map[string]any{},
		"required":   []string{},
	}
}

func (t *DiagnosticsTool) Execute(_ context.Context, _ map[string]any) *ToolResult {
	data, err := json.MarshalIndent(t.Report(), "", "  ")
	if err != nil {
		return ErrorResult("get_diagnostics: " + err.Error())
	}
	return SilentResult(string(data))
}

// Report builds the diagnostics document.
func (t *DiagnosticsTool) Report() map[string]any {
	now := t.nowFunc()
	var mem runtime.MemStats
	runtime.ReadMemStats(&mem)

	_, goVersion := config.FormatBuildInfo()
	report := map[string]any{
		"version":    config.GetVersion(),
		"git_commit": config.GitCommit,
		"go_version": goVersion,
		"os":         runtime.GOOS,
		"arch":       runtime.GOARCH,
		"num_cpu":    runtime.NumCPU(),
		"started_at": processStart.Format(time.RFC3339),
		"uptime_sec": int64(now.Sub(processStart) / time.Second),
		"memory": map[string]any{
			"heap_alloc_bytes": mem.HeapAlloc,
			"heap_sys_bytes":   mem.HeapSys,
			"sys_bytes":        mem.Sys,
			"num_gc":           mem.NumGC,
		},
		"goroutines": runtime.NumGoroutine(),
	}

	disk := map[string]any{}
	if free, total, err := diskSpace(t.workspace); err != nil {
		disk["error"] = err.Error()
	} else {
		disk["free_bytes"] = free
		disk["total_bytes"] = total
	}
	report["workspace_disk"] = disk

	if t.runtimeInfo != nil {
		maps.Copy(report, t.runtimeInfo())
	}
	return report
}
