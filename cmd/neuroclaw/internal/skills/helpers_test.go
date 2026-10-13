package skills

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"github.com/letrongminh/neuroclaw/pkg/config"
)

func TestSkillsInstallFromRegistryIsRefusedWhileRegistriesAreLocked(t *testing.T) {
	workspace := t.TempDir()
	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = workspace

	var server *httptest.Server
	server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/api/v3/repos/foo/bar":
			require.NoError(t, json.NewEncoder(w).Encode(map[string]any{"default_branch": "master"}))
		case "/api/v3/repos/foo/bar/contents/.agents/skills/pr-review":
			assert.Equal(t, "ref=master", r.URL.RawQuery)
			require.NoError(t, json.NewEncoder(w).Encode([]map[string]any{{
				"type":         "file",
				"name":         "SKILL.md",
				"download_url": server.URL + "/raw/foo/bar/master/.agents/skills/pr-review/SKILL.md",
			}}))
		case "/raw/foo/bar/master/.agents/skills/pr-review/SKILL.md":
			_, _ = w.Write([]byte("---\nname: pr-review\ndescription: PR review skill\n---\n# PR Review\n"))
		default:
			http.NotFound(w, r)
		}
	}))
	defer server.Close()

	githubRegistry, ok := cfg.Tools.Skills.Registries.Get("github")
	require.True(t, ok)
	githubRegistry.BaseURL = server.URL
	cfg.Tools.Skills.Registries.Set("github", githubRegistry)

	target := server.URL + "/foo/bar/tree/master/.agents/skills/pr-review"
	// NeuroClaw lockdown: skill registries are off, so nothing is fetched or written.
	// Skills come with the binary or are copied into the workspace by the operator.
	err := skillsInstallFromRegistry(cfg, "github", target)
	require.Error(t, err)
	assert.Contains(t, err.Error(), "not enabled")
	_, statErr := os.Stat(filepath.Join(workspace, "skills", "pr-review"))
	assert.True(t, os.IsNotExist(statErr), "a skill was written to the workspace")
}

func TestSkillsRemoveFromWorkspaceRejectsDotTarget(t *testing.T) {
	workspace := t.TempDir()
	skillsDir := filepath.Join(workspace, "skills")
	require.NoError(t, os.MkdirAll(skillsDir, 0o755))
	require.NoError(t, os.WriteFile(filepath.Join(skillsDir, "keep.txt"), []byte("keep"), 0o644))

	err := skillsRemoveFromWorkspace(workspace, config.DefaultConfig().Tools.Skills, ".")
	require.Error(t, err)
	assert.Contains(t, err.Error(), "invalid skill name")

	_, statErr := os.Stat(skillsDir)
	assert.NoError(t, statErr)
	_, fileErr := os.Stat(filepath.Join(skillsDir, "keep.txt"))
	assert.NoError(t, fileErr)
}

func TestSkillsRemoveFromWorkspaceUsesLastPathSegment(t *testing.T) {
	workspace := t.TempDir()
	targetDir := filepath.Join(workspace, "skills", "pr-review")
	require.NoError(t, os.MkdirAll(targetDir, 0o755))

	err := skillsRemoveFromWorkspace(
		workspace,
		config.DefaultConfig().Tools.Skills,
		"https://github.com/foo/bar/tree/main/.agents/skills/pr-review",
	)
	require.NoError(t, err)

	_, statErr := os.Stat(targetDir)
	assert.True(t, os.IsNotExist(statErr))
}

func TestSkillsRemoveFromWorkspaceSupportsRepoRootGitHubBlobURL(t *testing.T) {
	workspace := t.TempDir()
	targetDir := filepath.Join(workspace, "skills", "bar")
	require.NoError(t, os.MkdirAll(targetDir, 0o755))

	err := skillsRemoveFromWorkspace(
		workspace,
		config.DefaultConfig().Tools.Skills,
		"https://github.com/foo/bar/blob/feature/skills-registry/SKILL.md",
	)
	require.NoError(t, err)

	_, statErr := os.Stat(targetDir)
	assert.True(t, os.IsNotExist(statErr))
}

func TestSkillsRemoveFromWorkspaceSupportsGitHubEnterpriseURL(t *testing.T) {
	workspace := t.TempDir()
	targetDir := filepath.Join(workspace, "skills", "pr-review")
	require.NoError(t, os.MkdirAll(targetDir, 0o755))

	cfg := config.DefaultConfig()
	githubRegistry, ok := cfg.Tools.Skills.Registries.Get("github")
	require.True(t, ok)
	githubRegistry.BaseURL = "https://ghe.example.com/git"
	cfg.Tools.Skills.Registries.Set("github", githubRegistry)

	err := skillsRemoveFromWorkspace(
		workspace,
		cfg.Tools.Skills,
		"https://ghe.example.com/git/foo/bar/tree/main/.agents/skills/pr-review",
	)
	require.NoError(t, err)

	_, statErr := os.Stat(targetDir)
	assert.True(t, os.IsNotExist(statErr))
}

func TestSkillsRemoveFromWorkspaceDoesNotRequireEnabledGitHubRegistry(t *testing.T) {
	workspace := t.TempDir()
	targetDir := filepath.Join(workspace, "skills", "pr-review")
	require.NoError(t, os.MkdirAll(targetDir, 0o755))

	cfg := config.DefaultConfig()
	githubRegistry, ok := cfg.Tools.Skills.Registries.Get("github")
	require.True(t, ok)
	githubRegistry.Enabled = false
	cfg.Tools.Skills.Registries.Set("github", githubRegistry)

	err := skillsRemoveFromWorkspace(
		workspace,
		cfg.Tools.Skills,
		"https://github.com/foo/bar/tree/main/.agents/skills/pr-review",
	)
	require.NoError(t, err)

	_, statErr := os.Stat(targetDir)
	assert.True(t, os.IsNotExist(statErr))
}
