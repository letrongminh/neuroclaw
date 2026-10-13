# 💬 Konfigurasi Aplikasi Sembang

> Kembali ke [README](../project/README.ms.md)

## 💬 Aplikasi Sembang

Berbual dengan neuroclaw anda melalui Telegram, Discord, WhatsApp, Matrix, QQ, DingTalk, LINE, WeCom, Feishu, Slack, IRC, OneBot, MQTT, MaixCam, atau Pico (protokol asli)

> **Nota**: Semua saluran berasaskan webhook (LINE, WeCom, dan sebagainya) diservis pada satu pelayan HTTP Gateway yang dikongsi (`gateway.host`:`gateway.port`, lalai `127.0.0.1:18790`). Tiada port khusus per saluran untuk dikonfigurasikan. Nota: Feishu menggunakan mod WebSocket/SDK dan tidak menggunakan pelayan HTTP webhook yang dikongsi.

> **Tetapkan `allow_from` pada setiap saluran.** `allow_from` yang kosong membenarkan sesiapa sahaja yang boleh mencapai bot untuk berbual dengannya (gateway hanya mencatat amaran `SECURITY: Channel allows EVERYONE`). Senaraikan ID pengguna anda sendiri, atau guna `["*"]` untuk menyatakan secara jelas bahawa akses terbuka memang dimaksudkan. Beberapa contoh di bawah menunjukkan `"allow_from": []` demi ringkas; jangan biarkan kosong pada bot yang boleh menjalankan tool. Apa yang boleh dibuat oleh pengguna sembang terhadap agen (shell, arahan berjadual, pemasangan skill) disenaraikan dengan label risikonya dalam [Keupayaan §2](../reference/capabilities.md).

| Saluran          | Penyediaan                                 |
| ---------------- | ------------------------------------------ |
| **Telegram**     | Mudah (hanya token)                        |
| **Discord**      | Mudah (token bot + intents)                |
| **WhatsApp**     | Mudah (asli: imbas QR; atau bridge URL)    |
| **Matrix**       | Sederhana (homeserver + access token bot)  |
| **QQ**           | Mudah (AppID + AppSecret)                  |
| **DingTalk**     | Sederhana (kelayakan aplikasi)             |
| **LINE**         | Sederhana (kelayakan + webhook URL)        |
| **WeCom AI Bot** | Sederhana (bot_id + secret, WebSocket)     |
| **Feishu**       | Sederhana (App ID + Secret, mod WebSocket) |
| **Slack**        | Sederhana (Bot token + App token)          |
| **IRC**          | Sederhana (pelayan + konfigurasi TLS)      |
| **OneBot**       | Sederhana (QQ melalui protokol OneBot)     |
| **MQTT**         | Mudah (broker + agent_id)                  |
| **MaixCam**      | Mudah (integrasi perkakasan Sipeed)        |
| **Pico**         | Protokol NeuroClaw asli                     |

<details>
<summary><b>Telegram</b> (Disyorkan)</summary>

**1. Cipta bot**

* Buka Telegram, cari `@BotFather`
* Hantar `/newbot`, ikut arahan
* Salin token

**2. Konfigurasi**

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"],
      "use_markdown_v2": false,
    }
  }
}
```

> Dapatkan user ID anda daripada `@userinfobot` di Telegram.

**3. Jalankan**

```bash
neuroclaw gateway
```

**4. Menu arahan Telegram (auto-register semasa startup)**

NeuroClaw menyimpan definisi arahan dalam satu registry bersama. Semasa startup, Telegram akan mendaftarkan arahan bot yang disokong secara automatik (contohnya `/start`, `/help`, `/show`, `/list`, `/use`, `/btw`) supaya menu arahan dan tingkah laku runtime sentiasa selari.
Pendaftaran menu arahan Telegram kekal sebagai UX penemuan setempat saluran; pelaksanaan arahan generik dikendalikan secara berpusat dalam gelung agen melalui commands executor.

Jika pendaftaran arahan gagal (ralat sementara rangkaian/API), saluran tetap akan bermula dan NeuroClaw akan mencuba semula pendaftaran di latar belakang.

Anda juga boleh mengurus skill yang dipasang terus dari Telegram:

- `/list skills`
- `/use <skill> <message>`
- `/use <skill>` kemudian hantar permintaan sebenar dalam mesej seterusnya
- `/use clear`
- `/btw <question>` untuk bertanya soalan sampingan segera tanpa mengubah sejarah sesi aktif; `/btw` dikendalikan sebagai pertanyaan langsung tanpa tool dan tidak memasuki aliran pelaksanaan tool biasa

**5. Pemformatan Lanjutan**
Anda boleh menetapkan `use_markdown_v2: true` untuk mengaktifkan pilihan pemformatan yang lebih maju. Ini membolehkan bot menggunakan keseluruhan set ciri Telegram MarkdownV2, termasuk gaya bersarang, spoiler, dan blok lebar tetap tersuai.

</details>

<details>
<summary><b>Discord</b></summary>

**1. Cipta bot**

* Pergi ke <https://discord.com/developers/applications>
* Cipta aplikasi → Bot → Add Bot
* Salin token bot

**2. Aktifkan intents**

* Dalam tetapan Bot, aktifkan **MESSAGE CONTENT INTENT**
* (Pilihan) Aktifkan **SERVER MEMBERS INTENT** jika anda bercadang menggunakan allow list berasaskan data ahli

**3. Dapatkan User ID anda**
* Discord Settings → Advanced → aktifkan **Developer Mode**
* Klik kanan avatar anda → **Copy User ID**

**4. Konfigurasi**

```json
{
  "channel_list": {
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**5. Jemput bot**

* OAuth2 → URL Generator
* Scopes: `bot`
* Bot Permissions: `Send Messages`, `Read Message History`
* Buka URL jemputan yang dijana dan tambahkan bot ke pelayan anda

**Pilihan: Mod trigger kumpulan**

Secara lalai bot membalas semua mesej dalam saluran pelayan. Untuk mengehadkan balasan kepada @mention sahaja, tambah:

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "mention_only": true }
    }
  }
}
```

Anda juga boleh mencetuskan dengan awalan kata kunci (contohnya `!bot`):

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "prefixes": ["!bot"] }
    }
  }
}
```

**6. Jalankan**

```bash
neuroclaw gateway
```

</details>

<details>
<summary><b>WhatsApp</b> (asli melalui whatsmeow)</summary>

NeuroClaw boleh menyambung ke WhatsApp dalam dua cara:

- **Asli (disyorkan):** Dalam proses menggunakan [whatsmeow](https://github.com/tulir/whatsmeow). Tiada bridge berasingan. Tetapkan `"use_native": true` dan biarkan `bridge_url` kosong. Pada larian pertama, imbas kod QR dengan WhatsApp (Linked Devices). Sesi disimpan di bawah workspace anda (contohnya `workspace/whatsapp/`). Saluran asli ini adalah **pilihan** untuk memastikan binari lalai kekal kecil; bina dengan `-tags whatsapp_native` (contohnya `make build-whatsapp-native` atau `go build -tags whatsapp_native ./cmd/...`).
- **Bridge:** Sambung ke bridge WebSocket luaran. Tetapkan `bridge_url` (contohnya `ws://localhost:3001`) dan biarkan `use_native` sebagai false.

**Konfigurasi (asli)**

```json
{
  "channel_list": {
    "whatsapp": {
      "enabled": true,
      "type": "whatsapp",
      "use_native": true,
      "session_store_path": "",
      "allow_from": []
    }
  }
}
```

Jika `session_store_path` kosong, sesi akan disimpan dalam `<workspace>/whatsapp/`. Jalankan `neuroclaw gateway`; pada larian pertama, imbas kod QR yang dipaparkan dalam terminal menggunakan WhatsApp → Linked Devices.

</details>

<details>
<summary><b>QQ</b></summary>

**1. Cipta bot**

- Pergi ke [QQ Open Platform](https://q.qq.com/#)
- Cipta aplikasi → Dapatkan **AppID** dan **AppSecret**

**2. Konfigurasi**

```json
{
  "channel_list": {
    "qq": {
      "enabled": true,
      "type": "qq",
      "app_id": "YOUR_APP_ID",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

> Nyatakan nombor QQ dalam `allow_from` untuk mengehadkan akses. Kosong membenarkan semua pengguna; gunakan `["*"]` untuk menjadikannya jelas.

**3. Jalankan**

```bash
neuroclaw gateway
```

</details>

<details>
<summary><b>DingTalk</b></summary>

**1. Cipta bot**

* Pergi ke [Open Platform](https://open.dingtalk.com/)
* Cipta aplikasi dalaman
* Salin Client ID dan Client Secret

**2. Konfigurasi**

```json
{
  "channel_list": {
    "dingtalk": {
      "enabled": true,
      "type": "dingtalk",
      "client_id": "YOUR_CLIENT_ID",
      "client_secret": "YOUR_CLIENT_SECRET",
      "allow_from": []
    }
  }
}
```

> Nyatakan user ID DingTalk dalam `allow_from` untuk mengehadkan akses. Kosong membenarkan semua pengguna; gunakan `["*"]` untuk menjadikannya jelas.

**3. Jalankan**

```bash
neuroclaw gateway
```
</details>

<details>
<summary><b>Matrix</b></summary>

**1. Sediakan akaun bot**

* Gunakan homeserver pilihan anda (contohnya `https://matrix.org` atau self-hosted)
* Cipta pengguna bot dan dapatkan access tokennya

**2. Konfigurasi**

```json
{
  "channel_list": {
    "matrix": {
      "enabled": true,
      "type": "matrix",
      "homeserver": "https://matrix.org",
      "user_id": "@your-bot:matrix.org",
      "access_token": "YOUR_MATRIX_ACCESS_TOKEN",
      "allow_from": []
    }
  }
}
```

**3. Jalankan**

```bash
neuroclaw gateway
```

Untuk pilihan penuh (`device_id`, `join_on_invite`, `group_trigger`, `placeholder`, `reasoning_channel_id`), lihat [Panduan Konfigurasi Saluran Matrix](../channels/matrix/README.md).

</details>

<details>
<summary><b>LINE</b></summary>

**1. Cipta Akaun Rasmi LINE**

- Pergi ke [LINE Developers Console](https://developers.line.biz/)
- Cipta provider → Cipta saluran Messaging API
- Salin **Channel Secret** dan **Channel Access Token**

**2. Konfigurasi**

```json
{
  "channel_list": {
    "line": {
      "enabled": true,
      "type": "line",
      "channel_secret": "YOUR_CHANNEL_SECRET",
      "channel_access_token": "YOUR_CHANNEL_ACCESS_TOKEN",
      "webhook_path": "/webhook/line",
      "allow_from": []
    }
  }
}
```

> Webhook LINE diservis pada pelayan Gateway yang dikongsi (`gateway.host`:`gateway.port`, lalai `127.0.0.1:18790`).

**3. Tetapkan Webhook URL**

LINE memerlukan HTTPS untuk webhook. Gunakan reverse proxy atau tunnel:

```bash
# Contoh dengan ngrok (port lalai gateway ialah 18790)
ngrok http 18790
```

Kemudian tetapkan Webhook URL dalam LINE Developers Console kepada `https://your-domain/webhook/line` dan aktifkan **Use webhook**.

**4. Jalankan**

```bash
neuroclaw gateway
```

> Dalam sembang kumpulan, bot hanya membalas apabila @disebut. Balasan akan memetik mesej asal.

</details>

<details>
<summary><b>WeCom (企业微信)</b></summary>

NeuroClaw mendedahkan WeCom sebagai satu saluran AI Bot melalui WebSocket.
URL panggilan balik webhook awam tidak diperlukan.

Lihat [Panduan Konfigurasi WeCom](../channels/wecom/README.md) untuk rujukan konfigurasi penuh dan nota migrasi.

**Persediaan pantas - disyorkan**

**1. Sahkan**

```bash
neuroclaw auth wecom
```

Arahan ini memaparkan kod QR, menunggu kelulusan dalam WeCom, dan menulis `bot_id` + `secret` ke dalam entri `wecom` dalam `channel_list`.

**2. Konfigurasi secara manual jika perlu**

```json
{
  "channel_list": {
    "wecom": {
      "enabled": true,
      "type": "wecom",
      "bot_id": "YOUR_BOT_ID",
      "secret": "YOUR_SECRET",
      "websocket_url": "wss://openws.work.weixin.qq.com",
      "send_thinking_message": true,
      "allow_from": [],
      "reasoning_channel_id": ""
    }
  }
}
```

**3. Jalankan**

```bash
neuroclaw gateway
```

> Entri lama `wecom_app` dan `wecom_aibot` digantikan oleh entri `wecom` bersepadu di bawah `channel_list`.

</details>

<a id="mqtt"></a>
<details>
<summary><b>MQTT</b></summary>

Mana-mana client MQTT boleh berkomunikasi dengan NeuroClaw melalui broker. Peranti atau perkhidmatan menerbitkan permintaan ke broker; NeuroClaw melanggan, memproses dan menerbitkan respons kembali.

**1. Konfigurasi**

```json
{
  "channel_list": {
    "mqtt": {
      "enabled": true,
      "type": "mqtt",
      "settings": {
        "broker": "ssl://your-broker:8883",
        "agent_id": "assistant",
        "topic_prefix": "/neuroclaw",
        "keep_alive": 60,
        "qos": 0
      }
    }
  }
}
```

Nama pengguna dan kata laluan dalam `~/.neuroclaw/.security.yml`:

```yaml
channel_list:
  mqtt:
    settings:
      username: nama_pengguna
      password: kata_laluan
```

**Format topik**

```
{prefix}/{agent_id}/{client_id}/request    # Client → NeuroClaw
{prefix}/{agent_id}/{client_id}/response   # NeuroClaw → Client
```

`client_id` ditetapkan oleh aplikasi client anda untuk mengenal pasti peranti atau sesi.

**2. Jalankan**

```bash
neuroclaw gateway
```

**3. Uji**

```bash
mosquitto_pub -t "/neuroclaw/assistant/device1/request" \
  -m '{"text": "Helo"}'

mosquitto_sub -t "/neuroclaw/assistant/device1/response"
```

Untuk semua pilihan konfigurasi, lihat [Dokumentasi Saluran MQTT](../channels/mqtt/README.md).

</details>
