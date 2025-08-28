#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>

const char *ssid = "manucian";
const char *password = "0394143687";
const char *mqtt_server = "broker.hivemq.com";
const char *mqtt_user = "";
const char *mqtt_pass = "";

WiFiClient espClient;
PubSubClient client(espClient);

#define DHTPIN 33
#define DIEUHOA 32
#define QUAT 26
#define DEN 27
#define DHTTYPE DHT11 // Định nghĩa loại cảm biến là DHT11
DHT dht(DHTPIN, DHTTYPE);
long lastMsg = 0;
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);
  // Điều khiển các thiết bị dựa trên chủ đề và tin nhắn nhận được
  if (String(topic) == "esp32/dieuhoa")
  {
    if (message == "ON")
    {
      digitalWrite(DIEUHOA, HIGH);
      Serial.println("Điều hòa bật");
      client.publish("esp32/dieuhoaStatus", "ON");
    }
    else if (message == "OFF")
    {
      digitalWrite(DIEUHOA, LOW);
      Serial.println("Điều hòa tắt");
      client.publish("esp32/dieuhoaStatus", "OFF");
    }
  }
  if (String(topic) == "esp32/quat")
  {
    if (message == "ON")
    {
      digitalWrite(QUAT, HIGH);
      Serial.println("Quạt bật");
      client.publish("esp32/quatStatus", "ON");
    }
    else if (message == "OFF")
    {
      digitalWrite(QUAT, LOW);
      Serial.println("Quạt tắt");
      client.publish("esp32/quatStatus", "OFF");
    }
  }
  if (String(topic) == "esp32/den")
  {
    if (message == "ON")
    {
      digitalWrite(DEN, HIGH);
      Serial.println("Đèn bật");
      client.publish("esp32/denStatus", "ON");
    }
    else if (message == "OFF")
    {
      digitalWrite(DEN, LOW);
      Serial.println("Đèn tắt");
      client.publish("esp32/denStatus", "OFF");
    }
  }
  if (String(topic) == "esp32/turnall")
  {
    if (message == "ON")
    {
      digitalWrite(DIEUHOA, HIGH);
      digitalWrite(QUAT, HIGH);
      digitalWrite(DEN, HIGH);
    }
    else if (message == "OFF")
    {
      digitalWrite(DIEUHOA, LOW);
      digitalWrite(QUAT, LOW);
      digitalWrite(DEN, LOW);
    }
  }
}
void setup()
{
  Wire.begin();
  
  Serial.begin(115200);
  Serial.println("ESP32 IoT System Starting...");
  
  // Khởi tạo DHT với delay
  dht.begin();
  delay(2000); // Cho DHT11 thời gian khởi động
  
  // Test đọc DHT ngay trong setup
  Serial.println("Testing DHT11 sensor...");
  float testH = dht.readHumidity();
  float testT = dht.readTemperature();
  
  if (isnan(testH) || isnan(testT)) {
    Serial.println("WARNING: DHT11 not responding! Check wiring.");
  } else {
    Serial.println("DHT11 sensor OK!");
    Serial.print("Initial Temperature: ");
    Serial.println(testT);
    Serial.print("Initial Humidity: ");
    Serial.println(testH);
  }

  // Thiết lập các chân điều khiển làm output
  pinMode(DIEUHOA, OUTPUT);
  pinMode(QUAT, OUTPUT);
  pinMode(DEN, OUTPUT);

  // Khởi động Wi-Fi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.setKeepAlive(60);
  client.setSocketTimeout(30);
  
  Serial.println("MQTT setup completed");
}


void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    
    // Tạo unique client ID để tránh conflict
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("connected");
      Serial.println("Subscribing to topics...");
      
      // Đăng ký các chủ đề
      client.subscribe("esp32/dieuhoa");
      client.subscribe("esp32/quat");
      client.subscribe("esp32/den");
      client.subscribe("esp32/turnall");
      
      Serial.println("Subscribed to all topics");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  
  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
    
    // Đọc cảm biến DHT
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t))
    {
      Serial.println("ERROR: Failed to read from DHT sensor!");
      Serial.println("Check DHT11 wiring on GPIO33");
      return;
    }
    
    // Đọc cảm biến ánh sáng
    int lightValue = analogRead(36);
    
    // Hiển thị dữ liệu đẹp
    Serial.println("=== Sensor Data ===");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println("°C");
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println("%");
    Serial.print("Light: ");
    Serial.println(lightValue);
    
    // Tạo chuỗi dữ liệu MQTT
    String dataString = "Temperature: " + String(t) + " *C, Humidity: " + String(h) + " %, Light: " + String(lightValue);
    
    // Publish và kiểm tra kết quả
    bool publishResult = client.publish("esp32/datasensor", dataString.c_str());
    
    if (publishResult) {
      Serial.println("✓ Data published to MQTT successfully");
    } else {
      Serial.println("✗ Failed to publish data to MQTT");
    }
    
    Serial.println("========================");
    Serial.println();
  }
}