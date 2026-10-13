# NeuroClaw 🦞

Semua yang dilakukan [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 — aplikasi chat, penyedia
LLM, memori, pengingat, shell, skill, MCP, konsol web dan launcher — serta otak dari rumah
[NeuroEdge](https://github.com/letrongminh/neuroedge-init). Saat terhubung ke kotak rumah NeuroEdge,
ia menjangkau perangkat di rumah lewat tool MCP milik kotak itu, yang memutuskan berdasarkan aturan rumah
dan, untuk apa pun yang tidak bisa dibatalkan, bertanya dulu kepada Anda di rumah. Semua yang membuat
model bisa menjangkau lebih jauh dari chat (shell, bus perangkat keras, instalasi skill, hook, server MCP
stdio) menyala atau mati sesuai konfigurasi Anda, dan setiap yang menyala disebut saat startup (`risk=…`).

Jalankan di papan Linux, server (NeuroEdge Cloud), Mac, atau PC Windows; rilisnya mencakup platform yang
sama dengan PicoClaw. Instalasi: [install.md](../guides/install.md) (bahasa Inggris).

Atau langsung di chip: [`esp32/`](../../esp32/) adalah **NeuroClaw-ESP**, otak yang sama yang ditulis ulang
dalam C sebagai komponen ESP-IDF untuk satu ESP32-S3 — Telegram, LLM (dengan fallback), memori, pengingat,
heartbeat, pencarian web, pesan dan berkas ke pemilik, serta gate di chip seharga beberapa dolar, dengan
workspace dan tool yang sama seperti di sini ([apa yang sama, apa yang tidak](../../esp32/README.md)),
diberi label `risk=single_chip` karena otak dan gate berbagi satu ruang alamat.

NeuroClaw adalah hard fork dari [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). Lihat
[NOTICE](../../NOTICE).

## Apa yang berubah dari PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP lewat HTTP | Hanya header | Juga TLS timbal balik: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. Kotak NeuroEdge (`neuroedge mcp serve --http`) mewajibkan sertifikat klien, sehingga tidak perlu jembatan. |
| Shell, perangkat keras, instalasi skill, hook, MCP stdio | Menyala atau mati lewat konfigurasi | Sama, dengan bawaan PicoClaw; setiap yang menyala dicatat saat startup sebagai `NeuroClaw risk=<name>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`; [`pkg/config/risk.go`](../../pkg/config/risk.go)). Rumah NeuroEdge yang ingin otaknya tertutup menyatakannya di `config.json`. |
| Gate yang berkata "tanya pemilik" | Hasil tool biasa | Saat tool kotak mengembalikan `BLOCK` dengan `confirmation`, NeuroClaw memberi tahu model dengan jelas bahwa tidak ada yang dijalankan, bahwa hanya pemilik di rumah yang bisa menyetujui, dan agar tidak mencoba lagi ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | Mengunduh rilis `sipeed/picoclaw` | Mengunduh rilis repositori ini, diperiksa terhadap checksum-nya |
| Nama | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Selebihnya adalah milik PicoClaw: kanal chat (Telegram dan lainnya), penyedia LLM, memori dan sesi,
cron dan heartbeat, pencarian web, pesan, skill bawaan, subagen, konsol web dan launcher
(`neuroclaw-launcher`), image Docker (`ghcr.io/letrongminh/neuroclaw`), build nightly, paket deb/rpm,
dan DMG macOS. Panduannya ada di [`docs/`](../) ([README asli](PICOCLAW_README.md)); baca `picoclaw`
sebagai `neuroclaw` di sana.

**[Apa yang bisa dilakukan NeuroClaw](../reference/capabilities.md)** mendaftar setiap kemampuan kedua
otak, apa yang menyala secara bawaan, arti setiap label risiko, dan apa yang diketahui masih salah.

## Menghubungkan ke kotak NeuroEdge

Panduan: [neuroedge.md](../guides/neuroedge.md) (bahasa Inggris).

Kotak menjalankan `neuroedge mcp serve --http --ui …`; kuncinya berasal dari
`demo/i2f-chat-den-cua/tao-khoa.py` milik NeuroEdge. Di `~/.neuroclaw/config.json`:

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

`neuroclaw mcp test neuroedge` seharusnya mencetak `reachable (3 tools)` untuk agen demo.

## Build

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` membangun konsol web (`web/`, butuh pnpm); `make build-all` dan goreleaser
(`.goreleaser.yaml`) membangun semua platform. Di macOS tiga tes tool shell gagal seperti di PicoClaw
v0.3.1; CI berjalan di Linux, tempat tes itu lulus.

**Memperbarui dari build NeuroClaw sebelum label risiko** (2026-10-13): build tersebut memaksa shell,
perintah cron, instalasi skill, dan pemantauan USB mati, dan `onboard` menyimpannya dalam keadaan mati di
`config.json`. Nyalakan kembali di sana, atau mulai lagi dari bawaan dengan `neuroclaw config reset`.

## Lisensi

MIT, seperti PicoClaw. Copyright (c) 2026 PicoClaw contributors dan NeuroClaw contributors.
