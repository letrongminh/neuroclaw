# Depuração do NeuroClaw

> Voltar ao [README](../project/README.pt-br.md)

O NeuroClaw realiza múltiplas interações complexas nos bastidores para cada requisição que recebe — desde o roteamento de mensagens e avaliação de complexidade, até a execução de ferramentas e adaptação a falhas de modelo. Poder ver exatamente o que está acontecendo é crucial, não apenas para solucionar problemas potenciais, mas também para realmente entender como o agente opera.

## Iniciando o NeuroClaw em modo de depuração

Para obter informações detalhadas sobre o que o agente está fazendo (requisições LLM, chamadas de ferramentas, roteamento de mensagens), você pode iniciar o gateway do NeuroClaw com a flag de depuração:

```bash
neuroclaw gateway --debug
# or
neuroclaw gateway -d
```

Neste modo, o sistema formata os logs de forma detalhada e exibe prévias dos prompts do sistema e dos resultados de execução das ferramentas.

## Rótulos de risco na inicialização

Com ou sem `--debug`, `gateway` e `agent` registram um aviso `NeuroClaw risk=<name>` para cada capacidade arriscada que esteja habilitada (por exemplo `risk=shell`). Procure por eles primeiro quando precisar saber o que o modelo pode fazer nesta máquina. Nomes e significados: [capabilities.md seção 2](../reference/capabilities.md#2-risk-labels).

## Desabilitando a truncagem de logs (logs completos)

Por padrão, o NeuroClaw trunca strings muito longas (como o *Prompt do Sistema* ou resultados JSON grandes) nos logs de depuração para manter o console legível.

Se você precisar inspecionar a saída completa de um comando ou o payload exato enviado ao modelo LLM, pode usar a flag `--no-truncate`.

**Nota:** Esta flag *só* funciona quando combinada com o modo `--debug`.

```bash
neuroclaw gateway --debug --no-truncate

```

Quando esta flag está ativa, a função de truncagem global é desabilitada. Isso é extremamente útil para:

* Verificar a sintaxe exata das mensagens enviadas ao provedor.
* Ler a saída completa de ferramentas como `exec`, `web_fetch` ou `read_file`.
* Depurar o histórico de sessão salvo na memória.
