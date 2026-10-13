# NeuroClaw 🦞

Segala yang dilakukan [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 — aplikasi sembang,
penyedia LLM, memori, peringatan, shell, skill, MCP, konsol web dan launcher — serta otak bagi rumah
[NeuroEdge](https://github.com/letrongminh/neuroedge-init). Apabila disambungkan ke kotak rumah NeuroEdge,
ia mencapai peranti di rumah melalui tool MCP kotak itu, yang membuat keputusan mengikut peraturan rumah
dan, bagi apa-apa yang tidak boleh dibatalkan, bertanya kepada anda di rumah dahulu. Apa-apa yang
membolehkan model mencapai lebih jauh daripada sembang (shell, bas perkakasan, pemasangan skill, hook,
pelayan MCP stdio) dihidupkan atau dimatikan mengikut konfigurasi anda, dan setiap satu yang dihidupkan
dinamakan semasa permulaan (`risk=…`).

Jalankan pada papan Linux, pelayan (NeuroEdge Cloud), Mac atau PC Windows; keluaran merangkumi platform
yang sama seperti PicoClaw. Pemasangan: [install.md](../guides/install.md) (bahasa Inggeris).

Atau pada cip itu sendiri: [`esp32/`](../../esp32/) ialah **NeuroClaw-ESP**, otak yang sama ditulis semula
dalam C sebagai komponen ESP-IDF untuk satu ESP32-S3 — Telegram, LLM (dengan fallback), memori, peringatan,
heartbeat, carian web, mesej dan fail kepada pemilik, serta gate pada cip berharga beberapa dolar, dengan
workspace dan tool yang sama seperti di sini ([apa yang sama, apa yang tidak](../../esp32/README.md)),
berlabel `risk=single_chip` kerana otak dan gate berkongsi satu ruang alamat.

NeuroClaw ialah hard fork bagi [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). Lihat
[NOTICE](../../NOTICE).

## Apa yang berubah daripada PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP melalui HTTP | Header sahaja | Juga TLS bersama: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. Kotak NeuroEdge (`neuroedge mcp serve --http`) mewajibkan sijil klien, jadi tiada jambatan diperlukan. |
| Shell, perkakasan, pemasangan skill, hook, MCP stdio | Hidup atau mati mengikut konfigurasi | Sama, dengan nilai lalai PicoClaw; setiap satu yang dihidupkan dilog semasa permulaan sebagai `NeuroClaw risk=<name>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`; [`pkg/config/risk.go`](../../pkg/config/risk.go)). Rumah NeuroEdge yang mahu otaknya tertutup menyatakannya dalam `config.json`. |
| Gate yang berkata "tanya pemilik" | Keputusan tool biasa | Apabila tool kotak memulangkan `BLOCK` dengan `confirmation`, NeuroClaw memberitahu model dengan jelas bahawa tiada apa yang dijalankan, bahawa hanya pemilik di rumah boleh meluluskan, dan supaya tidak mencuba semula ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | Memuat turun keluaran `sipeed/picoclaw` | Memuat turun keluaran repositori ini, disemak dengan checksum-nya |
| Nama | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Selebihnya milik PicoClaw: saluran sembang (Telegram dan lain-lain), penyedia LLM, memori dan sesi,
cron dan heartbeat, carian web, mesej, skill terbina dalam, subejen, konsol web dan launcher
(`neuroclaw-launcher`), imej Docker (`ghcr.io/letrongminh/neuroclaw`), binaan nightly, pakej deb/rpm
dan DMG macOS. Panduannya ada dalam [`docs/`](../) ([README asal](PICOCLAW_README.md)); baca `picoclaw`
sebagai `neuroclaw` di sana.

**[Apa yang NeuroClaw boleh lakukan](../reference/capabilities.md)** menyenaraikan setiap keupayaan kedua-dua
otak, apa yang dihidupkan secara lalai, maksud setiap label risiko, dan apa yang diketahui masih salah.

## Sambung ke kotak NeuroEdge

Panduan: [neuroedge.md](../guides/neuroedge.md) (bahasa Inggeris).

Kotak menjalankan `neuroedge mcp serve --http --ui …`; kuncinya datang daripada
`demo/i2f-chat-den-cua/tao-khoa.py` milik NeuroEdge. Dalam `~/.neuroclaw/config.json`:

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

`neuroclaw mcp test neuroedge` sepatutnya mencetak `reachable (3 tools)` untuk ejen demo.

## Bina

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` membina konsol web (`web/`, perlukan pnpm); `make build-all` dan goreleaser
(`.goreleaser.yaml`) membina semua platform. Pada macOS tiga ujian tool shell gagal seperti dalam PicoClaw
v0.3.1; CI berjalan pada Linux, tempat ujian itu lulus.

**Menaik taraf daripada binaan NeuroClaw sebelum label risiko** (2026-10-13): binaan itu memaksa shell,
arahan cron, pemasangan skill dan pemantauan USB dimatikan, dan `onboard` menyimpannya dalam keadaan mati
dalam `config.json`. Hidupkannya di sana, atau mula semula daripada nilai lalai dengan `neuroclaw config reset`.

## Lesen

MIT, seperti PicoClaw. Hak cipta (c) 2026 PicoClaw contributors dan NeuroClaw contributors.
