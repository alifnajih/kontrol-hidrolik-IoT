# kontrol-hidrolik-IoT
membuat mesin kontrol hidrolik yang dapat di kontrol dengan menggunakan aplikasi android yang di buat dengan menggunakan APP Inventor

## 🔹 Fungsi Utama Program

Program ini dibuat untuk **kontrol perangkat listrik (relay)** dan **pengaturan kecepatan motor** lewat **Firebase Realtime Database** menggunakan **ESP8266**.

Fitur utama:

1. **WiFiManager** → memudahkan konfigurasi WiFi + Firebase URL & Secret langsung lewat web captive portal.
2. **EEPROM** → menyimpan konfigurasi Firebase supaya tidak hilang saat restart.
3. **Kontrol relay (4 buah)** → status relay diambil dari node `/IoT` di Firebase (`L1`, `L2`, `L3`, `L4`).
4. **Kontrol kecepatan motor** → membaca nilai `/IoT/kecepatan_motor` dari Firebase dan mengubah kecepatan motor dengan cara “menekan” tombol **PLUS** dan **MINUS** yang dikendalikan oleh pin ESP8266.
5. **Tombol reset WiFi** → jika ditekan > 3 detik, WiFiManager direset dan ESP restart.

---

## 🔹 Struktur Pin

* **Relay:**

  * Relay1 → D5 (GPIO14)
  * Relay2 → D1 (GPIO5)
  * Relay3 → D2 (GPIO4)
  * Relay4 → D7 (GPIO13)
* **Kontrol kecepatan motor:**

  * BTN_PLUS → D6 (GPIO12)
  * BTN_MINUS → D0 (GPIO16)
* **Tombol reset WiFi:**

  * D4 (GPIO2)

---

## 🔹 EEPROM Handling

* EEPROM digunakan untuk menyimpan:

  * Firebase URL (`firebase_url`) → mulai dari alamat 1.
  * Firebase Secret (`firebase_secret`) → mulai dari alamat 101.
* **Flag validasi EEPROM** → disimpan di alamat `0`. Jika nilainya `123`, artinya data valid.

Fungsi:

* `simpanKeEEPROM()` → menyimpan data Firebase jika ada perubahan.
* `bacaDariEEPROM()` → membaca data saat startup.
* `dataEEPROMValid()` → cek apakah flag EEPROM valid.

---

## 🔹 Alur Setup

1. Inisialisasi serial, relay, tombol, EEPROM.
2. Membaca data Firebase dari EEPROM.
3. Membuka captive portal WiFiManager dengan tambahan field untuk Firebase URL & Secret.

   * SSID AP: `"Bakar"`
   * Password: `"12345678"`
4. Jika sukses, data URL & Secret disimpan ke EEPROM.
5. Koneksi ke Firebase menggunakan legacy token.

---

## 🔹 Loop Utama

1. **Cek tombol reset WiFi** (D4 ditekan > 3 detik → reset konfigurasi).
2. **Kontrol relay**:

   * Ambil JSON dari path `/IoT` di Firebase.
   * Parsing JSON (`L1`, `L2`, `L3`, `L4`).
   * Update relay → LOW = aktif (ON), HIGH = mati (OFF).
3. **Kontrol motor**:

   * Ambil string dari path `/IoT/kecepatan_motor`.
   * Konversi ke `int` lalu disimpan ke `targetSpeed`.
   * Fungsi `updateMotorSpeed()` akan menyesuaikan `currentSpeed` ke `targetSpeed` secara bertahap dengan delay `300 ms`.
   * Tombol `PLUS` atau `MINUS` dipicu dengan fungsi `pressButton()`.

---

## 🔹 Mekanisme Kontrol Motor

* Variabel `currentSpeed` → posisi kecepatan motor saat ini.
* Variabel `targetSpeed` → nilai target dari Firebase.
* Fungsi `updateMotorSpeed()` → membandingkan `currentSpeed` dengan `targetSpeed`:

  * Jika `targetSpeed > currentSpeed` → menekan tombol `PLUS`.
  * Jika `targetSpeed < currentSpeed` → menekan tombol `MINUS`.
  * Jika sama → tidak melakukan apa-apa.

Contoh:

* `currentSpeed = 10`, `targetSpeed = 15` → program akan “menekan” tombol PLUS sebanyak 5 kali (bertahap, setiap 300 ms).
* `currentSpeed = 8`, `targetSpeed = 5` → program akan “menekan” tombol MINUS sebanyak 3 kali.

---

## 🔹 Kelebihan Program

1. **Fleksibel** → Firebase URL & Secret tidak hardcoded, bisa diubah lewat WiFiManager.
2. **Non-blocking motor control** → kecepatan motor berubah bertahap, tidak langsung meloncat.
3. **Relay control real-time** → dikontrol langsung dari Firebase.
4. **WiFi reset manual** → memudahkan saat ganti jaringan.

---

## 🔹 Contoh Struktur Data di Firebase

```json
"IoT": {
  "L1": 1,
  "L2": 0,
  "L3": 1,
  "L4": 0,
  "kecepatan_motor": "45"
}
```

Artinya:

* Relay1 = ON
* Relay2 = OFF
* Relay3 = ON
* Relay4 = OFF
* Motor target speed = 45

---

👉 Kesimpulan: program ini adalah **kontrol IoT berbasis Firebase** untuk relay + motor kecepatan bertahap dengan **konfigurasi dinamis** lewat WiFiManager dan **penyimpanan lokal** via EEPROM.
