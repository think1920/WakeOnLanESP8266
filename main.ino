#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>

// ================= CẤU HÌNH NGƯỜI DÙNG =================
const char* www_username = "admin";      // Tên đăng nhập Web
const char* www_password = "123";        // Mật khẩu Web

String apiKey = "xxx";                   // API Key OpenWeatherMap của bạn
String city = "Ho%20Chi%20Minh";         // Thành phố
String countryCode = "VN";               // Mã quốc gia
String Hostname = "wol-system";          // Tên thiết bị trong mạng LAN
// ========================================================

LiquidCrystal_I2C lcd(0x27, 20, 4); 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // Update giờ mỗi 60s

// Biến thời gian
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 1200000; // 20 phút (Tránh bị chặn API & giảm lag)

unsigned long lastScreenSwitch = 0;
unsigned long screenSwitchInterval = 10000; // Thời gian chờ chuyển màn hình
int currentScreen = 0;                      // 0: Weather, 1: System Info
unsigned long lastSecondUpdate = 0;

// Biến dữ liệu thời tiết
float temperature = 0;
int humidity = 0;
int pressure = 0;
float windSpeed = 0.0;

ESP8266WebServer server(80);
WiFiUDP UDP;
WakeOnLan WOL(UDP);

const char* pcFile = "/pc_list.json";

struct PC {
  String name;
  String mac;
  String ip;
};
std::vector<PC> pcList;

// HTML giao diện Web (Lưu trong Flash để tiết kiệm RAM)
const char htmlPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>WOL Control Panel</title>
    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css'>
    <link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css'>
    <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script>
    <script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js'></script>
    <style>body{padding-top:20px;background:#f4f6f9}.card{box-shadow:0 0 15px rgba(0,0,0,0.1);border:none}.btn-circle{width:30px;height:30px;padding:6px 0;border-radius:15px;text-align:center;font-size:12px;line-height:1.42857}</style>
    <script>
        $(document).ready(function() { updatePCList(); });
        function updatePCList() {
            fetch('/pc_list').then(r => r.json()).then(data => {
                const list = document.getElementById('pc-list'); list.innerHTML = '';
                data.forEach((pc, i) => {
                    list.innerHTML += `<li class="list-group-item d-flex justify-content-between align-items-center">
                        <div><i class="fas fa-desktop text-secondary mr-2"></i><strong>${pc.name}</strong><br><small class="text-muted ml-4">${pc.mac} | ${pc.ip}</small></div>
                        <div>
                            <button class="btn btn-outline-primary btn-sm mr-1" onclick="wakePC('${pc.mac}')" title="Wake Up"><i class="fas fa-power-off"></i> Wake</button>
                            <button class="btn btn-outline-secondary btn-sm" onclick="editPC(${i})" title="Edit"><i class="fas fa-cog"></i></button>
                        </div></li>`;
                });
            }).catch(e => console.error(e));
        }
        function apiRequest(url, data, successMsg) {
            fetch(url, { method: 'POST', headers: {'Content-Type':'application/json'}, body: JSON.stringify(data)})
            .then(r => { if(r.ok) { updatePCList(); $('#add-pc-modal').modal('hide'); $('#edit-pc-modal').modal('hide'); showNotif(successMsg, 'success'); } else showNotif('Error processing request', 'danger'); });
        }
        function addPC() {
            apiRequest('/add', { name: $('#pc-name').val(), mac: $('#pc-mac').val(), ip: $('#pc-ip').val() }, 'Device added successfully!');
        }
        function editPC(index) {
            fetch('/pc_list').then(r=>r.json()).then(data => {
                const pc = data[index];
                $('#edit-pc-name').val(pc.name); $('#edit-pc-mac').val(pc.mac); $('#edit-pc-ip').val(pc.ip);
                $('#edit-pc-modal').data('index', index).modal('show');
            });
        }
        function saveEditPC() {
            const index = $('#edit-pc-modal').data('index');
            apiRequest('/edit', { index, name: $('#edit-pc-name').val(), mac: $('#edit-pc-mac').val(), ip: $('#edit-pc-ip').val() }, 'Device updated!');
        }
        function confirmDelete() {
            if(!confirm("Are you sure you want to remove this device?")) return;
            const index = $('#edit-pc-modal').data('index');
            fetch('/delete?index=' + index, { method: 'POST' }).then(r => { if(r.ok) { updatePCList(); $('#edit-pc-modal').modal('hide'); showNotif('Device removed!', 'warning'); }});
        }
        function wakePC(mac) {
            fetch('/wake', { method: 'POST', headers: {'Content-Type':'application/json'}, body: JSON.stringify({ mac }) })
            .then(r => r.ok ? showNotif('Magic Packet sent to ' + mac, 'info') : showNotif('Failed to send packet', 'danger'));
        }
        function showNotif(msg, type) {
            const n = $(`<div class="alert alert-${type} alert-dismissible fade show shadow-sm"><strong>${type === 'success' ? 'Success!' : 'Notice:'}</strong> ${msg}<button type="button" class="close" data-dismiss="alert">&times;</button></div>`);
            $('#notification-area').append(n); setTimeout(() => n.alert('close'), 4000);
        }
        function resetWiFi() { fetch('/reset_wifi', {method:'POST'}).then(() => { alert('WiFi Settings Reset. Please reconnect to AP.'); location.reload(); }); }
    </script>
</head>
<body>
    <div class='container'>
        <div class='row justify-content-center'>
            <div class='col-md-8'>
                <h3 class='text-center mb-4 mt-2'><i class="fas fa-network-wired text-primary"></i> WOL Manager</h3>
                <div id='notification-area'></div>
                <div class='card'>
                    <div class='card-header bg-white d-flex justify-content-between align-items-center'>
                        <h5 class="mb-0">My Devices</h5>
                        <div>
                            <button class='btn btn-success btn-sm' data-toggle='modal' data-target='#add-pc-modal'><i class='fas fa-plus'></i> Add New</button>
                            <button class='btn btn-light btn-sm text-danger' onclick="if(confirm('Reset WiFi settings?')) resetWiFi()"><i class='fas fa-wifi'></i> Reset</button>
                        </div>
                    </div>
                    <ul id='pc-list' class='list-group list-group-flush'></ul>
                </div>
                <footer class='text-center mt-4 text-muted small'>&copy; 2026 TrienChill</footer>
            </div>
        </div>
    </div>
    
    <div class='modal fade' id='add-pc-modal'><div class='modal-dialog'><div class='modal-content'>
        <div class='modal-header'><h5 class='modal-title'>Add Device</h5><button class='close' data-dismiss='modal'>&times;</button></div>
        <div class='modal-body'>
            <div class="form-group"><label>Name</label><input type='text' class='form-control' id='pc-name' placeholder='Gaming PC'></div>
            <div class="form-group"><label>MAC Address</label><input type='text' class='form-control' id='pc-mac' placeholder='AA:BB:CC:DD:EE:FF'></div>
            <div class="form-group"><label>IP Address (Optional)</label><input type='text' class='form-control' id='pc-ip' placeholder='192.168.1.100'></div>
        </div>
        <div class='modal-footer'><button class='btn btn-secondary' data-dismiss='modal'>Cancel</button><button class='btn btn-primary' onclick='addPC()'>Add Device</button></div>
    </div></div></div>

    <div class='modal fade' id='edit-pc-modal'><div class='modal-dialog'><div class='modal-content'>
        <div class='modal-header'><h5 class='modal-title'>Edit Device</h5><button class='close' data-dismiss='modal'>&times;</button></div>
        <div class='modal-body'>
            <div class="form-group"><label>Name</label><input type='text' class='form-control' id='edit-pc-name'></div>
            <div class="form-group"><label>MAC Address</label><input type='text' class='form-control' id='edit-pc-mac'></div>
            <div class="form-group"><label>IP Address</label><input type='text' class='form-control' id='edit-pc-ip'></div>
        </div>
        <div class='modal-footer'>
            <button class='btn btn-danger mr-auto' onclick='confirmDelete()'><i class="fas fa-trash"></i> Delete</button>
            <button class='btn btn-primary' onclick='saveEditPC()'>Save Changes</button>
        </div>
    </div></div></div>
</body>
</html>
)=====";

// Hàm kiểm tra đăng nhập (Bảo mật)
bool checkAuth() {
  if (!server.authenticate(www_username, www_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// Đọc danh sách PC từ Flash
void loadPCData() {
  if (LittleFS.begin()) {
    if (LittleFS.exists(pcFile)) {
      File file = LittleFS.open(pcFile, "r");
      if (file) {
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, file);
        if (!error) {
          pcList.clear();
          for (JsonVariant v : doc.as<JsonArray>()) {
            PC pc;
            pc.name = v["name"].as<String>();
            pc.mac = v["mac"].as<String>();
            pc.ip = v["ip"].as<String>();
            pcList.push_back(pc);
          }
        }
        file.close();
      }
    }
  }
}

// Lưu danh sách PC vào Flash
void savePCData() {
  if (LittleFS.begin()) {
    File file = LittleFS.open(pcFile, "w");
    if (file) {
      StaticJsonDocument<2048> doc;
      JsonArray array = doc.to<JsonArray>();
      for (const PC& pc : pcList) {
        JsonObject obj = array.createNestedObject();
        obj["name"] = pc.name;
        obj["mac"] = pc.mac;
        obj["ip"] = pc.ip;
      }
      serializeJson(doc, file);
      file.close();
    }
  }
}

// ================= CÁC API HANDLE =================
void handleRoot() {
  if (!checkAuth()) return;
  server.send_P(200, "text/html", htmlPage);
}

void handlePCList() {
  if (!checkAuth()) return;
  String jsonResponse;
  StaticJsonDocument<2048> doc;
  JsonArray array = doc.to<JsonArray>();
  for (const PC& pc : pcList) {
    JsonObject obj = array.createNestedObject();
    obj["name"] = pc.name;
    obj["mac"] = pc.mac;
    obj["ip"] = pc.ip;  
  }
  serializeJson(doc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void handleAddPC() {
  if (!checkAuth()) return;
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    PC pc;
    pc.name = doc["name"].as<String>();
    pc.mac = doc["mac"].as<String>();
    pc.ip = doc["ip"].as<String>();  
    pcList.push_back(pc);
    savePCData();  
    server.send(200, "text/plain", "PC added");
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleEditPC() {
  if (!checkAuth()) return;
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    int index = doc["index"];
    if (index >= 0 && index < (int)pcList.size()) {
      PC& pc = pcList[index];
      pc.name = doc["name"].as<String>();
      pc.mac = doc["mac"].as<String>();
      pc.ip = doc["ip"].as<String>();  
      savePCData();                    
      server.send(200, "text/plain", "PC updated");
    } else {
      server.send(404, "text/plain", "PC not found");
    }
  }
}

void handleDeletePC() {
  if (!checkAuth()) return;
  if (server.method() == HTTP_POST) {
    int index = server.arg("index").toInt();
    if (index >= 0 && index < (int)pcList.size()) {
      pcList.erase(pcList.begin() + index);
      savePCData(); 
      server.send(200, "text/plain", "PC deleted");
    } else {
      server.send(404, "text/plain", "PC not found");
    }
  }
}

void handleWakePC() {
  if (!checkAuth()) return;
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    String mac = doc["mac"].as<String>();
    
    // Gửi WOL packet 3 lần liên tiếp để đảm bảo
    WOL.sendMagicPacket(mac.c_str()); delay(50);
    WOL.sendMagicPacket(mac.c_str()); delay(50);
    if (WOL.sendMagicPacket(mac.c_str())) {
      server.send(200, "text/plain", "WOL packet sent");
    } else {
      server.send(500, "text/plain", "Failed to send");
    }
  }
}

void resetWiFiSettings() {
  if (!checkAuth()) return;
  server.send(200, "text/plain", "Resetting...");
  delay(1000);
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

// Lấy dữ liệu thời tiết (Blocking ~1-2s)
void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";
    
    http.begin(client, url);
    int httpCode = http.GET(); 
    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        temperature = doc["main"]["temp"].as<float>();
        humidity = doc["main"]["humidity"].as<int>();
        pressure = doc["main"]["pressure"].as<int>();
        windSpeed = doc["wind"]["speed"].as<float>();
      }
    }
    http.end();
  }
}

// Vẽ khung tĩnh (Chỉ chạy 1 lần khi đổi màn hình để tránh nháy)
void drawScreenLayout(int screen) {
  lcd.clear(); 
  if (screen == 0) {
    lcd.setCursor(0, 0); lcd.print("T:     C");
    lcd.setCursor(0, 1); lcd.print("H:     %");
    lcd.setCursor(0, 2); lcd.print("P:     hPa");
    lcd.setCursor(0, 3); lcd.print("W:     m/s");
  } else {
    lcd.setCursor(0, 0); lcd.print("Time:");
    lcd.setCursor(0, 1); lcd.print("Date:");
    lcd.setCursor(0, 2); lcd.print("IP:");
    lcd.setCursor(0, 3); lcd.print("Free:");
  }
}

// Cập nhật giá trị động
void updateScreenValues() {
  if (currentScreen == 0) {
    lcd.setCursor(3, 0); lcd.print(temperature, 1);
    lcd.setCursor(3, 1); lcd.print(humidity); lcd.print(" "); 
    lcd.setCursor(3, 2); lcd.print(pressure);
    lcd.setCursor(3, 3); lcd.print(windSpeed, 1);
  } else {
    lcd.setCursor(6, 0); lcd.print(timeClient.getFormattedTime());
    
    time_t rawTime = timeClient.getEpochTime();
    struct tm * ti = localtime(&rawTime);
    
    lcd.setCursor(6, 1);
    if (ti->tm_mday < 10) lcd.print("0"); lcd.print(ti->tm_mday); lcd.print("/");
    if (ti->tm_mon+1 < 10) lcd.print("0"); lcd.print(ti->tm_mon + 1); lcd.print("/");
    lcd.print(1900 + ti->tm_year);

    lcd.setCursor(4, 2); lcd.print(WiFi.localIP());
    lcd.setCursor(6, 3); lcd.print(ESP.getFreeHeap()); lcd.print(" B   ");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Khởi động màn hình LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("System Booting...");
  
  // Khởi động File System và Load Data
  LittleFS.begin();
  loadPCData();
  
  // Khởi động UDP cho Wake On Lan
  UDP.begin(9); 
  
  // Kết nối WiFi (Tự động tạo AP nếu chưa có pass)
  WiFi.hostname(Hostname);
  WiFiManager wifiManager;
  wifiManager.setTimeout(180); // Timeout 3 phút
  
  // Hiển thị hướng dẫn lên LCD khi đang đợi WiFi
  lcd.setCursor(0, 1); lcd.print("Connect AP:");
  lcd.setCursor(0, 2); lcd.print("WOL-ESP8266");
  
  if(!wifiManager.autoConnect("WOL-ESP8266")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();
  }

  // Định tuyến Web Server
  server.on("/", handleRoot);
  server.on("/pc_list", handlePCList);
  server.on("/add", handleAddPC);
  server.on("/edit", handleEditPC);
  server.on("/delete", handleDeletePC);
  server.on("/wake", handleWakePC);
  server.on("/reset_wifi", resetWiFiSettings);
  
  server.begin();

  // Khởi động NTP & Lấy thời tiết lần đầu
  timeClient.begin();
  fetchWeather();
  
  // Setup màn hình hiển thị đầu tiên
  drawScreenLayout(currentScreen);
  updateScreenValues();
  
  // Random khoảng thời gian chuyển màn hình lần đầu
  screenSwitchInterval = random(5000, 15000);
}

void loop() {
  timeClient.update();
  server.handleClient();

  unsigned long currentMillis = millis();

  // 1. Cập nhật thời tiết (20 phút/lần)
  if (currentMillis - lastWeatherUpdate > weatherUpdateInterval) {
    fetchWeather();
    lastWeatherUpdate = currentMillis;
    if (currentScreen == 0) updateScreenValues(); // Update ngay nếu đang xem
  }

  // 2. Logic chuyển màn hình ngẫu nhiên (5-15s)
  if (currentMillis - lastScreenSwitch > screenSwitchInterval) {
    currentScreen = !currentScreen; // Đảo màn hình
    
    drawScreenLayout(currentScreen); // Vẽ khung mới
    updateScreenValues();            // Điền dữ liệu
    
    lastScreenSwitch = currentMillis;
    screenSwitchInterval = random(5000, 15000); // Random cho lần kế tiếp
  }

  // 3. Cập nhật giây đồng hồ (Chỉ chạy mỗi 1s khi ở màn hình System)
  if (currentScreen == 1 && currentMillis - lastSecondUpdate >= 1000) {
    lastSecondUpdate = currentMillis;
    updateScreenValues();
  }
  
  // 4. CỰC KỲ QUAN TRỌNG: Giúp ổn định WiFi và giảm nhiệt độ chip
  delay(1); 
}
