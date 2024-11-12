
# Capstone Smart PJU

Smart PJU merupakan sistem lampu Penerangan Jalan Umum yang dapat dikendalikan dan dimonitor dari jarak jauh. Sistem ini menitikberatkan pada penggunaan metode komunikasi jarak jauh menggunakan LoRa dan deteksi kerusakan lampu menggunakan LDR. Pada sistem ini, terdapat hardware berupa mikrokontroler yang terbagi menjadi empat jenis:

## Controller

Controller merupakan sebuah mikrokontroler yang terletak di sisi pengelola. Mikrokontroler ini akan terhubung dengan jaringan Wi-Fi sehingga dapat berkomunikasi dengan server website melalui protokol MQTT. Controller ini yang juga bertugas untuk mengirimkan pesan menggunakan LoRa ke lampu Anchor pada sebuah blok PJU.

## Anchor

Anchor merupakan mikrokontroler yang terpasang pada lampu PJU di suatu blok. Anchor memiliki tugas untuk menerima perintah dari Controller dan meneruskannya ke setiap noda lampu PJU yang ada pada blok tersebut. Selanjutnya Anchor akan menunggu pesan respon dari setiap noda lampu dan meneruskannya ke Controller untuk ditampilkan di website.

## Node

Node atau Noda lampu merupakan mikrokontroler yang terpasang pada lampu PJU di suatu blok. Node hanya dapat menerima perintah dari Anchor dan merespon kembali ketika perintah tersebut diterima.

## Backup Anchor

Backup Anchor merupakan mikrokontroler yang bekerja seperti node pada kondisi biasa, namun daput berubah menjadi Anchor ketika Anchor utama tersebut tidak merespon Controller (rusak). 
