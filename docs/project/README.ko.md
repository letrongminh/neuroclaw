# NeuroClaw 🦞

[PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 이 하는 모든 일(채팅 앱, LLM 제공자, 메모리, 리마인더, 셸, 스킬, MCP, 웹 콘솔과 런처)에 더해, [NeuroEdge](https://github.com/letrongminh/neuroedge-init) 홈의 두뇌로 동작합니다. NeuroEdge 홈 박스에 연결하면 박스의 MCP 도구를 통해 집 안의 기기에 접근합니다. 도구는 집의 규칙에 따라 판단하고, 되돌릴 수 없는 작업은 먼저 집에 있는 사용자에게 확인을 요청합니다. 모델이 채팅 밖으로 손을 뻗을 수 있게 하는 기능(셸, 하드웨어 버스, 스킬 설치, 훅, stdio MCP 서버)은 설정한 대로 켜지거나 꺼지며, 켜져 있는 기능은 시작할 때 이름이 출력됩니다(`risk=…`).

Linux 보드, 서버(NeuroEdge Cloud), Mac, Windows PC에서 실행할 수 있습니다. 릴리스가 지원하는 플랫폼은 PicoClaw와 같습니다. 설치: [docs/guides/install.md](../guides/install.md) (영어).

칩 위에서 직접 실행할 수도 있습니다. [`esp32/`](../../esp32/)는 **NeuroClaw-ESP**로, 같은 두뇌를 ESP-IDF 컴포넌트로 C로 다시 작성해 ESP32-S3 한 개에서 실행합니다. Telegram, LLM(폴백 포함), 메모리, 리마인더, 하트비트, 웹 검색, 소유자에게 보내는 메시지와 파일, 그리고 게이트까지 몇 달러짜리 칩에 담았으며, 워크스페이스와 도구도 여기와 같습니다([같은 점과 다른 점](../../esp32/README.md)). 두뇌와 게이트가 같은 주소 공간을 공유하므로 `risk=single_chip` 라벨이 붙습니다.

NeuroClaw는 [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1(MIT)의 하드 포크입니다. [NOTICE](../../NOTICE)를 참고하세요.

## PicoClaw에서 달라진 점

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| HTTP 기반 MCP | 헤더만 지원 | 상호 TLS도 지원: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. NeuroEdge 박스(`neuroedge mcp serve --http`)는 클라이언트 인증서를 요구하므로 브리지가 필요 없습니다. |
| 셸, 하드웨어, 스킬 설치, 훅, stdio MCP | 설정으로 켜고 끔 | 동일하며 기본값도 PicoClaw와 같습니다. 켜져 있는 항목은 시작할 때 `NeuroClaw risk=<name>`으로 기록됩니다(`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`. [`pkg/config/risk.go`](../../pkg/config/risk.go)). 두뇌를 닫아 두려는 NeuroEdge 홈은 `config.json`에 그렇게 지정합니다. |
| "소유자에게 물어보라"고 말하는 게이트 | 일반 도구 결과 | 박스 도구가 `confirmation`이 있는 `BLOCK`을 반환하면, NeuroClaw는 모델에게 아무것도 실행되지 않았고, 승인할 수 있는 사람은 집에 있는 소유자뿐이며, 다시 시도하지 말라고 분명히 알려 줍니다([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | `sipeed/picoclaw` 릴리스를 다운로드 | 이 저장소의 릴리스를 다운로드하고 체크섬으로 확인 |
| 이름 | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

나머지는 모두 PicoClaw의 것입니다. 채팅 채널(Telegram 등), LLM 제공자, 메모리와 세션, cron과 하트비트, 웹 검색, 메시지, 기본 제공 스킬, 서브에이전트, 웹 콘솔과 런처(`neuroclaw-launcher`), Docker 이미지(`ghcr.io/letrongminh/neuroclaw`), 나이틀리 빌드, deb/rpm 패키지, macOS DMG. 가이드는 [`docs/`](../)에 있습니다([원본 README](PICOCLAW_README.md)). 그곳에서는 `picoclaw`를 `neuroclaw`로 바꿔 읽으세요.

**[NeuroClaw가 할 수 있는 일](../reference/capabilities.md)**(영어)에는 두 두뇌의 모든 기능, 기본으로 켜져 있는 것, 각 리스크 라벨의 의미, 알려진 문제가 정리되어 있습니다.

## NeuroEdge 박스에 연결하기

가이드: [docs/guides/neuroedge.md](../guides/neuroedge.md) (영어).

박스에서는 `neuroedge mcp serve --http --ui …`를 실행하며, 키는 NeuroEdge의 `demo/i2f-chat-den-cua/tao-khoa.py`로 만듭니다. `~/.neuroclaw/config.json`에 다음과 같이 씁니다.

```json
"tools": {
  "mcp": {
    "enabled": true,
    "servers": {
      "neuroedge": {
        "enabled": true,
        "type": "http",
        "url": "https://home-box.example:8443/mcp",
        "headers": { "Authorization": "Bearer <the brain's token>" },
        "tls": {
          "ca_file": "~/neuroedge-keys/ca.pem",
          "cert_file": "~/neuroedge-keys/nao.pem",
          "key_file": "~/neuroedge-keys/nao.key"
        }
      }
    }
  }
}
```

데모 에이전트에서는 `neuroclaw mcp test neuroedge`가 `reachable (3 tools)`를 출력해야 합니다.

## 빌드

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher`는 웹 콘솔(`web/`, pnpm 필요)을 빌드합니다. `make build-all`과 goreleaser(`.goreleaser.yaml`)는 모든 플랫폼을 빌드합니다. macOS에서는 PicoClaw v0.3.1과 마찬가지로 셸 도구 테스트 3개가 실패합니다. CI는 Linux에서 실행되며 거기서는 통과합니다.

**리스크 라벨 이전의 NeuroClaw 빌드에서 업그레이드하는 경우**(2026-10-13): 그 빌드들은 셸, cron 명령, 스킬 설치, USB 모니터링을 강제로 끄고 `onboard`가 꺼진 상태로 `config.json`에 저장했습니다. `config.json`에서 다시 켜거나, `neuroclaw config reset`으로 기본값에서 다시 시작하세요.

## 라이선스

MIT(PicoClaw와 동일). Copyright (c) 2026 PicoClaw contributors and NeuroClaw contributors.
