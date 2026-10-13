# Security

Security-focused docs covering configuration, secrets handling, and provider auth.

- [Security Configuration](security_configuration.md): security-related config knobs and hardening guidance.
- [Sensitive Data Filtering](sensitive_data_filtering.md): filtering secrets from tool output before model use.
- [Credential Encryption](credential_encryption.md): encrypting stored API keys and credentials.
- Risk labels: at startup `gateway` and `agent` log one `NeuroClaw risk=<name>` warning for each capability that is on (shell, scheduled shell, hardware buses, skill install, process hooks, USB monitor, stdio MCP). The table is [capabilities.md section 2](../reference/capabilities.md#2-risk-labels); how a NeuroEdge home fits around them is in the [NeuroEdge guide](../guides/neuroedge.md).
- [Antigravity Authentication & Integration Guide](ANTIGRAVITY_AUTH.md): auth flow and integration notes for the Antigravity provider.
