#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <HX711.h>

// 08:3A:F2:6E:7B:40
uint8_t gatewayMacAddress[] = {0x08, 0x3A, 0xF2, 0x6E, 0x7B, 0x40};

constexpr char WIFI_SSID[] = "HUAWEI nova 5T";

// ID Node
const int this_node = 1;

// Sensor Ultrasonic
const int ECHO_PIN = 25;
const int TRIG_PIN = 26;

// Load Cell
const int SCK_PIN = 18;
const int DT_PIN = 19;
HX711 scale;
float calibration_factor = 92.50; //109.00;
int gram;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

//
const int min_tinggi = 5; // 3 cm penuh

typedef struct pesan
{
  int node;
  float tinggi;
  int berat;
} pesan;

pesan dataPesan;

int32_t getWiFiChannel(const char *ssid);
void connectESPNow();
double hitungTinggi();
int hitungBerat();
void tampilLCD(int tinggi);

void setup()
{
  Serial.begin(9600);

  connectESPNow();

  delay(1000);

  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);

  scale.begin(DT_PIN, SCK_PIN);
  scale.tare();

  lcd.init();
  lcd.backlight();

  lcd.print("NODE ");
  lcd.print(this_node);
}

void loop()
{
  lcd.clear();

  dataPesan.node = this_node;
  dataPesan.tinggi = hitungTinggi();
  dataPesan.berat = hitungBerat();

  esp_err_t result = esp_now_send(gatewayMacAddress, (uint8_t *)&dataPesan, sizeof(dataPesan));
  result == ESP_OK ? Serial.println("Pesan berhasil dikirim") : Serial.println("Pesan gagal dikirim");

  tampilLCD(dataPesan.tinggi);

  Serial.print("Tinggi : ");
  Serial.println(dataPesan.tinggi);

  Serial.print("Berat : ");
  Serial.println(dataPesan.berat);

  delay(2000);
}

int32_t getWiFiChannel(const char *ssid)
{
  if (int32_t n = WiFi.scanNetworks())
  {
    for (uint8_t i = 0; i < n; i++)
    {
      if (!strcmp(ssid, WiFi.SSID(i).c_str()))
      {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

void connectESPNow()
{
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Terjadi masalah inisialisasi ESP-NOW");
    return;
  }

  esp_now_peer_info_t gatewayInfo;
  memcpy(gatewayInfo.peer_addr, gatewayMacAddress, 6);
  gatewayInfo.channel = 0;
  gatewayInfo.encrypt = false;

  if (esp_now_add_peer(&gatewayInfo) != ESP_OK)
  {
    Serial.println("Terjadi masalah saat registrasi gateway");
    return;
  }
}

double hitungTinggi()
{
  double durasi, jarakCM;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  durasi = pulseIn(ECHO_PIN, HIGH);

  // Konversi durasi pulsein ke cm
  jarakCM = durasi / 29 / 2;

  return jarakCM;
}

int hitungBerat()
{
  if (scale.is_ready())
  {
    scale.set_scale(calibration_factor);
    gram = scale.get_units();
    return gram >= 0 ? gram : 0;
  }
  Serial.println("HX711 tidak terhubung");
  return 0;
}

void tampilLCD(int tinggi)
{
  lcd.setCursor(0, 0);
  if (tinggi > min_tinggi)
  {
    lcd.print("Tidak Penuh");
  }
  else
  {
    lcd.print("Penuh");
  }
}