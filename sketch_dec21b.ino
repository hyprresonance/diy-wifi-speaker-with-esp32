#include <WiFi.h>
#include <WebServer.h>
#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

HardwareSerial dfSerial(2);  // UART2 for DFPlayer
DFRobotDFPlayerMini player;

WebServer server(80);

const char* ssid = "nguyentri";
const char* password = "nguyentri";

String currentSong = "0001.mp3";
int volumeLevel = 20;
bool isPlaying = false;

void setup() {
  Serial.begin(115200);

  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  if (!player.begin(dfSerial)) {
    Serial.println("DFPlayer Error!");
    while (true);
  }
  player.volume(volumeLevel);
  player.play(1);
  isPlaying = true;

  Wire.begin(21, 22);  // I2C for OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Error!");
    while (true);
  }
  updateDisplay();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.print("IP: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);  
  server.on("/play", [](){ player.start(); isPlaying = true; updateDisplay(); server.send(200); });
  server.on("/pause", [](){ player.pause(); isPlaying = false; updateDisplay(); server.send(200); });
  server.on("/next", [](){ player.next(); updateDisplay(); server.send(200); });
  server.on("/prev", [](){ player.previous(); updateDisplay(); server.send(200); });
  server.on("/volup", [](){ volumeLevel = min(30, volumeLevel + 3); player.volume(volumeLevel); updateDisplay(); server.send(200); });
  server.on("/voldown", [](){ volumeLevel = max(0, volumeLevel - 3); player.volume(volumeLevel); updateDisplay(); server.send(200); });

  server.begin();
}

void loop() {
  server.handleClient();
  if (player.available() && player.readType() == DFPlayerPlayFinished) {
  int fileNum = player.readCurrentFileNumber();
  currentSong = "Track " + String(fileNum); 
  updateDisplay();
}
}

void updateDisplay() {
  int currentNum = player.readCurrentFileNumber(); 
  if (currentNum == -1) currentNum = 1; 

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("Playing Track:");
  display.setTextSize(2); 
  display.println("#" + String(currentNum)); 
  
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.println("Vol: " + String(volumeLevel) + "/30");
  display.println("Mode: " + String(isPlaying ? "Playing" : "Paused"));
  display.display();
}
void handleRoot() {
  String html = R"rawliteral(
  <html>
  <head><meta charset="UTF-8"><title>Loa WiFi</title>
  <style>body { background: #f0f0f0; text-align: center; font-family: Arial; }
  button { font-size: 1.5em; padding: 10px; margin: 5px; }</style>
  <script>
  function updateUI() {
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        document.getElementById('song').innerText = data.song;
        document.getElementById('vol').innerText = data.vol;
        document.getElementById('status').innerText = data.status;
      });
  }
  setInterval(updateUI, 2000);  // Update every 2s
  </script>
  </head>
  <body><h1>DFPlayer WiFi DIY Speaker</h1>
  <p>Song: <span id="song"></span></p>
  <p>Vol: <span id="vol"></span></p>
  <p>Status: <span id="status"></span></p>
  <button onclick="fetch('/play')">Play</button>
  <button onclick="fetch('/pause')">Pause</button>
  <button onclick="fetch('/next')">Next</button>
  <button onclick="fetch('/prev')">Prev</button>
  <button onclick="fetch('/volup')">Vol +</button>
  <button onclick="fetch('/voldown')">Vol -</button>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleStatus() {
  String json = "{\"song\":\"" + currentSong + "\", \"vol\":" + String(volumeLevel) + ", \"status\":\"" + String(isPlaying ? "Playing" : "Paused") + "\"}";
  server.send(200, "application/json", json);
}
