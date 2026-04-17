#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_AMG88xx.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>

// --- Display Pin Setup ---
#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_AMG88xx amg;

// --- Thermal Grid ---
float pixels[64];
const int gridSize = 32; // 32x32 interpolated
uint16_t colors[gridSize * gridSize];

// --- WiFi AP ---
const char* ssid = "ThermalCam";
const char* password = "12345678";
WebServer server(80);

// --- Interpolation ---
float getInterpolatedTemp(int x, int y) {
  float gx = (float)x * 7.0 / (gridSize - 1);
  float gy = (float)y * 7.0 / (gridSize - 1);
  int x0 = (int)gx;
  int y0 = (int)gy;
  int x1 = min(x0 + 1, 7);
  int y1 = min(y0 + 1, 7);
  float dx = gx - x0;
  float dy = gy - y0;
  float t00 = pixels[y0 * 8 + x0];
  float t01 = pixels[y0 * 8 + x1];
  float t10 = pixels[y1 * 8 + x0];
  float t11 = pixels[y1 * 8 + x1];
  float t0 = t00 + dx * (t01 - t00);
  float t1 = t10 + dx * (t11 - t10);
  return t0 + dy * (t1 - t0);
}

// --- Color Mapping (Temp to RGB565 + RGB888) ---
uint16_t mapTempToColor(float temp, uint8_t &r, uint8_t &g, uint8_t &b) {
  float tMin = 20.0, tMax = 40.0;
  temp = constrain(temp, tMin, tMax);
  uint8_t val = map((int)(temp * 10), tMin * 10, tMax * 10, 0, 255);
  if (val < 128) {
    r = 0;
    g = val * 2;
    b = 255 - val * 2;
  } else {
    r = (val - 128) * 2;
    g = 255 - (val - 128) * 2;
    b = 0;
  }
  return tft.color565(r, g, b);
}

// --- Web Page with Canvas ---
const char* htmlPage = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='utf-8'>
<title>ESP32 Thermal Camera</title>
<style>
canvas { image-rendering: pixelated; }
</style>
</head><body>
<h2>ESP32 Thermal Camera</h2>
<canvas id="c" width="32" height="32" style="width:320px;height:320px;"></canvas>
<script>
const canvas = document.getElementById("c");
const ctx = canvas.getContext("2d");
function drawHeatmap(data) {
  const img = ctx.getImageData(0, 0, 32, 32);
  for (let i = 0; i < data.length; i++) {
    const [r, g, b] = data[i];
    img.data[i * 4 + 0] = r;
    img.data[i * 4 + 1] = g;
    img.data[i * 4 + 2] = b;
    img.data[i * 4 + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
}
function update() {
  fetch("/heatmap")
    .then(res => res.json())
    .then(drawHeatmap);
}
setInterval(update, 500);
update();
</script>
</body></html>
)rawliteral";

// --- Web Handlers ---
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleHeatmap() {
  String json = "[";
  for (int i = 0; i < gridSize * gridSize; i++) {
    uint16_t color = colors[i];
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    // Convert RGB565 to 8-bit per channel
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;
    json += "[" + String(r) + "," + String(g) + "," + String(b) + "]";
    if (i < gridSize * gridSize - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // ESP32 I2C pins

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  if (!amg.begin()) {
    Serial.println("AMG8833 not found!");
    while (1);
  }

  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started. Connect to:");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/heatmap", handleHeatmap);
  server.begin();
}

// --- Main Loop ---
void loop() {
  amg.readPixels(pixels);
  int block = 128 / gridSize;

  for (int y = 0; y < gridSize; y++) {
    for (int x = 0; x < gridSize; x++) {
      float temp = getInterpolatedTemp(x, y);
      uint8_t r, g, b;
      uint16_t color = mapTempToColor(temp, r, g, b);
      colors[y * gridSize + x] = color;

      // Draw on TFT
      tft.fillRect(x * block, y * block, block, block, color);
    }
  }

  server.handleClient();
  delay(100);
}