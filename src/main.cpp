#include <WiFi.h>
// #include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>

// ================= WiFi =================
const char* ssid = "manucian";        // Tên WiFi
const char* password = "0394143687"; // Mật khẩu WiFi

// ================= MQTT (HiveMQ Cloud) =================
const char* mqtt_server = "192.22.18.104";
const int mqtt_port = 1883;             // TLS port
const char* mqtt_user = "user1";     // Username HiveMQ Cloud
const char* mqtt_pass = "123456";   // Password HiveMQ Cloud

// WiFiClientSecure espClient;
WiFiClient espClient; 
PubSubClient client(espClient);

// ================= DHT + IO =================
#define DHTPIN 33
#define DIEUHOA 32
#define QUAT 26
#define DEN 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ================= Biến thời gian =================
long lastMsg = 0;

// ================= Reconnect MQTT =================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối MQTT... ");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("thành công!");
      // Subscribe các topic (ESP32 đăng kí nhận tin nhắn) 
      client.subscribe("esp32/dieuhoa");
      client.subscribe("esp32/quat");
      client.subscribe("esp32/den");
      client.subscribe("esp32/turnall");
    } else {
      Serial.print("Thất bại, rc=");
      Serial.print(client.state());
      Serial.println(" thử lại sau 5s...");
      delay(5000);
    }
  }
}

// ================= Setup WiFi connection =================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Đang kết nối WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ================= Callback for handling MQTT messages =================
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == "esp32/dieuhoa") {
    if (message == "ON") {
      digitalWrite(DIEUHOA, HIGH);
      client.publish("esp32/dieuhoaStatus", "ON");
    } else {
      digitalWrite(DIEUHOA, LOW);
      client.publish("esp32/dieuhoaStatus", "OFF");
    }
  }

  if (String(topic) == "esp32/quat") {
    if (message == "ON") {
      digitalWrite(QUAT, HIGH);
      client.publish("esp32/quatStatus", "ON");
    } else {
      digitalWrite(QUAT, LOW);
      client.publish("esp32/quatStatus", "OFF");
    }
  }

  if (String(topic) == "esp32/den") {
    if (message == "ON") {
      digitalWrite(DEN, HIGH);
      client.publish("esp32/denStatus", "ON");
    } else {
      digitalWrite(DEN, LOW);
      client.publish("esp32/denStatus", "OFF");
    }
  }

  if (String(topic) == "esp32/turnall") {
    if (message == "ON") {
      digitalWrite(DEN, HIGH);
      digitalWrite(DIEUHOA, HIGH);
      digitalWrite(QUAT, HIGH);
    } else {
      digitalWrite(DEN, LOW);
      digitalWrite(DIEUHOA, LOW);
      digitalWrite(QUAT, LOW);
    }
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  dht.begin();

  // GPIO output
  pinMode(DIEUHOA, OUTPUT);
  pinMode(QUAT, OUTPUT);
  pinMode(DEN, OUTPUT);

  // WiFi
  setup_wifi();

  // Bỏ qua chứng chỉ TLS (chỉ để test)
  // espClient.setInsecure();

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ================= LOOP =================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Lỗi đọc DHT!");
      return;
    }

    int lightValue = analogRead(35);

    String dataString = "Temperature: " + String(t) + " *C, " +
                        "Humidity: " + String(h) + " %, " +
                        "Light: " + String(lightValue);

    client.publish("esp32/datasensor", dataString.c_str()); // Gửi dữ liệu lên MQTT
    Serial.println("Gửi dữ liệu: " + dataString);
  }
}