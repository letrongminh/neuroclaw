package providers

import (
	"context"
	"errors"
	"time"

	"github.com/letrongminh/neuroclaw/pkg/logger"
)

// budgetedProvider counts every request against an LLMBudget before it
// reaches the wrapped provider: chat, streaming, every fallback-chain
// candidate attempt, retries, summaries. Wrapping at the provider boundary is
// what makes the budget count each request, whichever code path makes it.
//
// The optional interfaces are kept: Close, SupportsThinking and
// SupportsNativeSearch delegate (false / no-op when the wrapped provider lacks
// them), and the streaming interfaces are only present when the wrapped
// provider has them (see WithLLMBudget).
type budgetedProvider struct {
	inner  LLMProvider
	budget *LLMBudget
}

// WithLLMBudget wraps p so that each request counts against b. It returns p
// unchanged when p or b is nil, or when p already counts against b.
func WithLLMBudget(p LLMProvider, b *LLMBudget) LLMProvider {
	if p == nil || b == nil {
		return p
	}
	if already, ok := p.(interface{ llmBudget() *LLMBudget }); ok && already.llmBudget() == b {
		return p
	}
	base := &budgetedProvider{inner: p, budget: b}
	_, streams := p.(StreamingProvider)
	_, events := p.(StreamingEventProvider)
	switch {
	case streams && events:
		return &budgetedStreamEventProvider{budgetedProvider: base}
	case streams:
		return &budgetedStreamProvider{budgetedProvider: base}
	case events:
		return &budgetedEventProvider{budgetedProvider: base}
	default:
		return base
	}
}

// UnwrapLLMBudget returns the provider under a budget wrapper, or p itself.
func UnwrapLLMBudget(p LLMProvider) LLMProvider {
	if w, ok := p.(interface{ unwrapLLMBudget() LLMProvider }); ok {
		return w.unwrapLLMBudget()
	}
	return p
}

func (p *budgetedProvider) llmBudget() *LLMBudget        { return p.budget }
func (p *budgetedProvider) unwrapLLMBudget() LLMProvider { return p.inner }

func (p *budgetedProvider) acquire(model string) error {
	err := p.budget.Acquire()
	if err != nil {
		fields := map[string]any{"model": model}
		var exceeded *LLMBudgetExceededError
		if errors.As(err, &exceeded) {
			fields["window"] = exceeded.Window
			fields["used"] = exceeded.Used
			fields["limit"] = exceeded.Limit
			fields["resets_at"] = exceeded.ResetAt.Format(time.RFC3339)
		}
		logger.WarnCF("agent", "NeuroClaw llm_budget", fields)
	}
	return err
}

func (p *budgetedProvider) Chat(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
) (*LLMResponse, error) {
	if err := p.acquire(model); err != nil {
		return nil, err
	}
	return p.inner.Chat(ctx, messages, tools, model, options)
}

func (p *budgetedProvider) GetDefaultModel() string { return p.inner.GetDefaultModel() }

func (p *budgetedProvider) Close() {
	if stateful, ok := p.inner.(StatefulProvider); ok {
		stateful.Close()
	}
}

func (p *budgetedProvider) SupportsThinking() bool {
	tc, ok := p.inner.(ThinkingCapable)
	return ok && tc.SupportsThinking()
}

func (p *budgetedProvider) SupportsNativeSearch() bool {
	ns, ok := p.inner.(NativeSearchCapable)
	return ok && ns.SupportsNativeSearch()
}

func (p *budgetedProvider) chatStream(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
	onChunk func(accumulated string),
) (*LLMResponse, error) {
	if err := p.acquire(model); err != nil {
		return nil, err
	}
	return p.inner.(StreamingProvider).ChatStream(ctx, messages, tools, model, options, onChunk)
}

func (p *budgetedProvider) chatStreamEvents(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
	onChunk func(StreamChunk),
) (*LLMResponse, error) {
	if err := p.acquire(model); err != nil {
		return nil, err
	}
	return p.inner.(StreamingEventProvider).ChatStreamEvents(ctx, messages, tools, model, options, onChunk)
}

type budgetedStreamProvider struct{ *budgetedProvider }

func (p *budgetedStreamProvider) ChatStream(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
	onChunk func(accumulated string),
) (*LLMResponse, error) {
	return p.chatStream(ctx, messages, tools, model, options, onChunk)
}

type budgetedEventProvider struct{ *budgetedProvider }

func (p *budgetedEventProvider) ChatStreamEvents(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
	onChunk func(StreamChunk),
) (*LLMResponse, error) {
	return p.chatStreamEvents(ctx, messages, tools, model, options, onChunk)
}

type budgetedStreamEventProvider struct{ *budgetedProvider }

func (p *budgetedStreamEventProvider) ChatStream(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
	onChunk func(accumulated string),
) (*LLMResponse, error) {
	return p.chatStream(ctx, messages, tools, model, options, onChunk)
}

func (p *budgetedStreamEventProvider) ChatStreamEvents(
	ctx context.Context,
	messages []Message,
	tools []ToolDefinition,
	model string,
	options map[string]any,
	onChunk func(StreamChunk),
) (*LLMResponse, error) {
	return p.chatStreamEvents(ctx, messages, tools, model, options, onChunk)
}
