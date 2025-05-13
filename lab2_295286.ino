#include <WiFi.h>
#include <FirebaseESP32.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuration
#define FIREBASE_HOST "lab2-295286.firebaseapp.com"
#define FIREBASE_API_KEY "AIzaSyBYP1RC-43W5-5QpRwXSlnuxT8nNVwFit0"
#define FIREBASE_DB_URL "https://lab2-295286-default-rtdb.asia-southeast1.firebasedatabase.app"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Global Variables
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

String ssid = "", password = "", id = "";
bool apmode = false;
unsigned long lastUpdateTime = 0;

// ================= Core Functions =================
bool hasCredentials() {
  return ssid.length() > 0 && password.length() > 0;
}

bool connectWiFi() {
  displayStatus("Connecting WiFi...");
  Serial.println("Attempting to connect to: " + ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected! IP: " + WiFi.localIP().toString());
      displayStatus("WiFi Connected");
      delay(5000);

      // Add server verification
      Serial.println("Starting web server...");
      server.begin();
      Serial.println("Web server started. Endpoints:");
      Serial.println("- http://" + WiFi.localIP().toString() + "/");
      return true;
    }
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("\nConnection failed");
  displayStatus("WiFi Failed");
  return false;
}

bool connectFirebase() {
  displayStatus("Connecting Firebase...");
  Serial.println("Initializing Firebase...");
  
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DB_URL;
  auth.user.email = "irdinaabalqiss@gmail.com";
  auth.user.password = "Dina123";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Waiting for Firebase...");
  delay(2000);

  if (!Firebase.ready()) {
    Serial.println("Firebase connection failed!");
    Serial.println("Reason: " + fbdo.errorReason());
    return false;
  }
  
  Serial.println("Firebase connected!");
  return true;
}

void displayFirebaseMessage() {
  String path = "/hello";

  if (Firebase.getString(fbdo, path)) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(fbdo.stringData());
    display.display();
  } else {
    displayStatus("Firebase Read Error");
  }
}

// ================= Mode Handlers =================
void startAPMode() {
  digitalWrite(2, HIGH);
  apmode = true;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_eeprom", "");
  displayStatus("AP Mode: 192.168.4.1");
  
  server.on("/", []() {
    String content = R"=====(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP32 WiFi Setup</title>
        <style>
            * {
                box-sizing: border-box;
                font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            }
            body {
                background-color: #f5f5f5;
                margin: 0;
                padding: 20px;
                display: flex;
                justify-content: center;
                align-items: center;
                min-height: 100vh;
                color: #333;
            }
            .container {
                background: white;
                border-radius: 10px;
                box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
                padding: 30px;
                width: 100%;
                max-width: 400px;
                text-align: center;
            }
            h1 {
                color: #2c3e50;
                margin-top: 0;
                font-size: 24px;
            }
            .logo {
                width: 60px;
                height: 60px;
                margin-bottom: 15px;
            }
            .form-group {
                margin-bottom: 20px;
                text-align: left;
            }
            label {
                display: block;
                margin-bottom: 5px;
                font-weight: 500;
                color: #555;
            }
            input {
                width: 100%;
                padding: 12px;
                border: 1px solid #ddd;
                border-radius: 5px;
                font-size: 16px;
                transition: border 0.3s;
            }
            input:focus {
                border-color: #3498db;
                outline: none;
                box-shadow: 0 0 0 2px rgba(52, 152, 219, 0.2);
            }
            button {
                background-color: #3498db;
                color: white;
                border: none;
                padding: 12px 20px;
                border-radius: 5px;
                font-size: 16px;
                cursor: pointer;
                width: 100%;
                font-weight: 500;
                transition: background-color 0.3s;
            }
            button:hover {
                background-color: #2980b9;
            }
            .status {
                margin-top: 20px;
                padding: 10px;
                background-color: #f8f9fa;
                border-radius: 5px;
                font-size: 14px;
            }
            @media (max-width: 480px) {
                .container {
                    padding: 20px;
                }
            }
        </style>
    </head>
    <body>
        <div class="container">
            <svg class="logo" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#3498db">
                <path d="M12 2L1 12h3v9h6v-6h4v6h6v-9h3L12 2zm0 2.8L18 10v9h-2v-6H8v6H6v-9l6-7.2z"/>
            </svg>
            <h1>WiFi Configuration</h1>
            <p>Please enter your network credentials</p>
            
            <form method="post" action="/save">
                <div class="form-group">
                    <label for="ssid">WiFi Network (SSID)</label>
                    <input type="text" id="ssid" name="ssid" placeholder="Your WiFi name" required>
                </div>
                
                <div class="form-group">
                    <label for="pass">Password</label>
                    <input type="password" id="pass" name="pass" placeholder="Your WiFi password">
                </div>
                
                <div class="form-group">
                    <label for="id">Device ID</label>
                    <input type="text" id="id" name="id" placeholder="Unique device identifier" required>
                </div>
                
                <button type="submit">Save & Reboot</button>
            </form>
            
            <div class="status">
                Connection IP: 192.168.4.1
            </div>
        </div>
    </body>
    </html>
    )=====";
    server.send(200, "text/html", content);
  });

  server.on("/save", []() {
    ssid = server.arg("ssid");
    password = server.arg("pass");
    id = server.arg("id");
    
    writeData(ssid, password, id);
    String content = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Settings Saved</title>
        <style>
            body {
                font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                background-color: #f5f5f5;
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
                margin: 0;
                text-align: center;
            }
            .message {
                background: white;
                padding: 30px;
                border-radius: 10px;
                box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
                max-width: 400px;
            }
            h1 {
                color: #27ae60;
                margin-top: 0;
            }
            .spinner {
                border: 4px solid rgba(0, 0, 0, 0.1);
                border-radius: 50%;
                border-top: 4px solid #3498db;
                width: 40px;
                height: 40px;
                animation: spin 1s linear infinite;
                margin: 20px auto;
            }
            @keyframes spin {
                0% { transform: rotate(0deg); }
                100% { transform: rotate(360deg); }
            }
        </style>
    </head>
    <body>
        <div class="message">
            <h1>Settings Saved Successfully!</h1>
        </div>
    </body>
    </html>
    )=====";
    server.send(200, "text/html", content);
    delay(2000);
    ESP.restart();
  });

  server.begin();
}
// ================= Global Styles Definition =================
const String STYLES = R"=====(
<style>
  :root {
    --primary: #4361ee;
    --secondary: #3a0ca3;
    --success: #4cc9f0;
    --warning: #f8961e;
    --danger: #f94144;
    --light: #f8f9fa;
    --dark: #212529;
  }
  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #f5f7fa 0%, #dfe7f5 100%);
    margin: 0;
    padding: 20px;
    min-height: 100vh;
    display: flex;
    justify-content: center;
    align-items: center;
  }
  .card {
    background: white;
    border-radius: 12px;
    box-shadow: 0 8px 30px rgba(0,0,0,0.1);
    padding: 2rem;
    width: 100%;
    max-width: 480px;
    transition: all 0.3s ease;
  }
  h1 {
    color: var(--primary);
    text-align: center;
    margin-bottom: 1.5rem;
    font-weight: 600;
  }
  .form-group {
    margin-bottom: 1.25rem;
  }
  label {
    display: block;
    margin-bottom: 0.5rem;
    color: var(--dark);
    font-weight: 500;
  }
  input, select {
    width: 100%;
    padding: 12px 6px;
    border: 2px solid #e9ecef;
    border-radius: 8px;
    font-size: 16px;
    transition: all 0.3s;
  }
  input:focus, select:focus {
    border-color: var(--primary);
    outline: none;
    box-shadow: 0 0 0 3px rgba(67, 97, 238, 0.2);
  }
  .btn {
    display: inline-block;
    padding: 12px 24px;
    border: none;
    border-radius: 8px;
    font-size: 16px;
    font-weight: 500;
    cursor: pointer;
    text-align: center;
    transition: all 0.3s;
  }
  .btn-block {
    display: block;
    width: 100%;
  }
  .btn-primary {
    background-color: var(--primary);
    color: white;
  }
  .btn-primary:hover {
    background-color: var(--secondary);
  }
  .btn-success {
    background-color: var(--success);
    color: white;
  }
  .btn-warning {
    background-color: var(--warning);
    color: white;
  }
  .btn-group {
    display: flex;
    gap: 12px;
    margin-top: 1.5rem;
  }
  .message-box {
    margin-top: 1.5rem;
    padding: 1rem;
    background-color: var(--light);
    border-radius: 8px;
    border-left: 4px solid var(--primary);
  }
  .hidden {
    display: none;
  }
  .text-center {
    text-align: center;
  }
  .mt-3 {
    margin-top: 1rem;
  }
  .device-id {
    font-weight: bold;
    color: var(--primary);
  }
</style>
)=====";

// ================= Updated launchUserUI() Function =================
void launchUserUI() {
  // Login Page (unchanged)
  server.on("/", []() {
    String content = String("<!DOCTYPE html><html><head>") +
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" +
      "<title>Device Manager - Login</title>" +
      STYLES +
      "</head><body>" +
      "<div class=\"card\">" +
      "<h1>Device Manager Login</h1>" +
      "<form method=\"post\" action=\"/auth\">" +
      "<div class=\"form-group\">" +
      "<label for=\"username\">Username</label>" +
      "<input type=\"text\" id=\"username\" name=\"username\" placeholder=\"Enter your username\" required>" +
      "</div>" +
      "<div class=\"form-group\">" +
      "<label for=\"pass\">Password</label>" +
      "<input type=\"password\" id=\"pass\" name=\"pass\" placeholder=\"Enter your password\" required>" +
      "</div>" +
      "<button type=\"submit\" class=\"btn btn-primary btn-block\">Login</button>" +
      "</form>" +
      "</div></body></html>";
    
    server.send(200, "text/html", content);
  });

  // Authentication Handler (unchanged)
  server.on("/auth", HTTP_POST, []() {
      String username = server.arg("username");
      String pass = server.arg("pass");
      
      String userPath = "/users/" + username;
      
      if (Firebase.getJSON(fbdo, userPath)) {
          FirebaseJson json = fbdo.jsonObject();
          FirebaseJsonData passwordData;
          
          if (json.get(passwordData, "password")) {
              if (passwordData.stringValue == pass) {
                  server.sendHeader("Location", "/home");
                  server.send(302);
              } else {
                  String errorContent = String("<!DOCTYPE html><html><head>") +
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" +
                    "<title>Login Failed</title>" +
                    STYLES +
                    "</head><body>" +
                    "<div class=\"card\">" +
                    "<h1 style=\"color: var(--danger);\">Login Failed</h1>" +
                    "<div class=\"message-box\">" +
                    "<p>Invalid username or password</p>" +
                    "</div>" +
                    "<a href=\"/\" class=\"btn btn-primary btn-block mt-3\">Try Again</a>" +
                    "</div></body></html>";
                  server.send(401, "text/html", errorContent);
              }
          } else {
              String errorContent = String("<!DOCTYPE html><html><head>") +
                "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" +
                "<title>Login Failed</title>" +
                STYLES +
                "</head><body>" +
                "<div class=\"card\">" +
                "<h1 style=\"color: var(--danger);\">Login Failed</h1>" +
                "<div class=\"message-box\">" +
                "<p>Password field not found</p>" +
                "</div>" +
                "<a href=\"/\" class=\"btn btn-primary btn-block mt-3\">Try Again</a>" +
                "</div></body></html>";
              server.send(401, "text/html", errorContent);
          }
      } else {
          String errorContent = String("<!DOCTYPE html><html><head>") +
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" +
            "<title>Login Failed</title>" +
            STYLES +
            "</head><body>" +
            "<div class=\"card\">" +
            "<h1 style=\"color: var(--danger);\">Login Failed</h1>" +
            "<div class=\"message-box\">" +
            "<p>User not found</p>" +
            "</div>" +
            "<a href=\"/\" class=\"btn btn-primary btn-block mt-3\">Try Again</a>" +
            "</div></body></html>";
          server.send(401, "text/html", errorContent);
      }
  });

  server.on("/home", HTTP_GET, []() {
  String homePage = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>Device Dashboard</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        padding-top: 50px;
        background-color: #f2f2f2;
      }
      h1 {
        color: #333;
      }
      .button {
        display: inline-block;
        margin: 20px;
        padding: 15px 30px;
        font-size: 18px;
        color: #fff;
        background-color: #3498db;
        border: none;
        border-radius: 6px;
        cursor: pointer;
        text-decoration: none;
        transition: 0.3s;
      }
      .button:hover {
        background-color: #2980b9;
      }
      .button.red {
        background-color: #e74c3c;
      }
      .button.red:hover {
        background-color: #c0392b;
      }
    </style>
  </head>
  <body>
    <h1>Welcome to Device Dashboard</h1>
    <a href="/select" class="button">Update Device Message</a>
    <form action="/reconfigure" method="POST" style="display:inline;">
      <button type="submit" class="button red">Reconfigure WiFi</button>
    </form>
  </body>
  </html>
  )=====";

  server.send(200, "text/html", homePage);
  });

  // Device Control Page (unchanged)
  server.on("/select", []() {
    String content = String("<!DOCTYPE html><html><head>") +
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" +
      "<title>Device Control Panel</title>" +
      STYLES +
      "<script>" +
      "function showMessage() {" +
      "  const deviceId = document.querySelector('[name=\"device_id\"]').value;" +
      "  fetch(`/getMessage?device_id=${deviceId}`)" +
      "    .then(response => response.json())" +
      "    .then(data => {" +
      "      document.getElementById('currentMessage').textContent = data.message;" +
      "      document.getElementById('messageDisplay').classList.remove('hidden');" +
      "      document.getElementById('updateForm').classList.add('hidden');" +
      "    });" +
      "}" +
      "function showUpdateForm() {" +
      "  document.getElementById('messageDisplay').classList.add('hidden');" +
      "  document.getElementById('updateForm').classList.remove('hidden');" +
      "}" +
      "function updateMessage() {" +
      "  const formData = new FormData(document.getElementById('deviceForm'));" +
      "  const params = new URLSearchParams(formData);" +
      "  fetch('/update', { method: 'POST', body: params })" +
      "    .then(response => response.text())" +
      "    .then(html => { document.body.innerHTML = html; });" +
      "}" +
      "</script>" +
      "</head><body>" +
      "<div class=\"card\">" +
      "<h1>Device Control Panel</h1>" +
      "<form id=\"deviceForm\">" +
      "<div class=\"form-group\">" +
      "<label for=\"device_id\">Select Device</label>" +
      "<select id=\"device_id\" name=\"device_id\" required>" +
      "<option value=\"\" disabled selected>Choose a device</option>" +
      "<option value=\"101\">Device 101</option>" +
      "<option value=\"102\">Device 102</option>" +
      "<option value=\"103\">Device 103</option>" +
      "</select>" +
      "</div>" +
      "<div class=\"btn-group\">" +
      "<button type=\"button\" class=\"btn btn-success\" onclick=\"showMessage()\">Show Message</button>" +
      "<button type=\"button\" class=\"btn btn-warning\" onclick=\"showUpdateForm()\">Update Message</button>" +
      "</div>" +
      "<div id=\"messageDisplay\" class=\"message-box hidden\">" +
      "<h3>Current Message:</h3>" +
      "<p id=\"currentMessage\"></p>" +
      "</div>" +
      "<div id=\"updateForm\" class=\"hidden\">" +
      "<div class=\"form-group\">" +
      "<label for=\"new_message\">New Message</label>" +
      "<input type=\"text\" id=\"new_message\" name=\"new_message\" placeholder=\"Enter new message\" required>" +
      "</div>" +
      "<button type=\"button\" class=\"btn btn-primary btn-block\" onclick=\"updateMessage()\">Submit Update</button>" +
      "</div>" +
      "</form>" +
      "</div></body></html>";
    
    server.send(200, "text/html", content);
  });

  server.on("/reconfigure", HTTP_POST, []() {
    String selectedSSID = server.arg("ssid");
    String customSSID = server.arg("custom_ssid");
    String password = server.arg("password");

    String finalSSID = (customSSID != "") ? customSSID : selectedSSID;

    EEPROM.writeString(0, finalSSID);
    EEPROM.writeString(50, password);
    EEPROM.commit();

    String html = String("<!DOCTYPE html>")
           + String("<html lang='en'>")
           + "<head>"
           + "<meta charset='UTF-8'>"
           + "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
           + "<title>Rebooting...</title>"
           + "<style>"
           + "body {"
           + "    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;"
           + "    background-color: #f0f2f5;"
           + "    margin: 0;"
           + "    display: flex;"
           + "    justify-content: center;"
           + "    align-items: center;"
           + "    height: 100vh;"
           + "}"
           + ".container {"
           + "    background: white;"
           + "    padding: 30px 40px;"
           + "    border-radius: 12px;"
           + "    box-shadow: 0 4px 15px rgba(0,0,0,0.1);"
           + "    text-align: center;"
           + "    animation: fadeIn 1s ease-in;"
           + "}"
           + "h2 {"
           + "    color: #007BFF;"
           + "    margin-bottom: 20px;"
           + "}"
           + "p {"
           + "    font-size: 16px;"
           + "    color: #333;"
           + "    line-height: 1.6;"
           + "}"
           + "a.button {"
           + "    display: inline-block;"
           + "    margin-top: 15px;"
           + "    padding: 10px 20px;"
           + "    background-color: #007BFF;"
           + "    color: white;"
           + "    text-decoration: none;"
           + "    border-radius: 5px;"
           + "    transition: background-color 0.3s;"
           + "}"
           + "a.button:hover {"
           + "    background-color: #0056b3;"
           + "}"
           + "@keyframes fadeIn {"
           + "    from { opacity: 0; transform: translateY(10px); }"
           + "    to { opacity: 1; transform: translateY(0); }"
           + "}"
           + "</style>"
           + "</head>"
           + "<body>"
           + "<div class='container'>"
           + "<h2>Rebooting Now...</h2>"
           + "<p>Please wait for the blue lamp on the ESP board to light up.<br>"
           + "Connect to the <strong>ESP32_eeprom</strong> Wi-Fi network,<br>then visit the following page:</p>"
           + "<a class='button' href='http://192.168.4.1' target='_blank'>Go to Setup Page</a>"
           + "</div>"
           + "</body>"
           + "</html>";


    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  });

  server.on("/reconfigure", HTTP_GET, []() {
    String currentSSID = EEPROM.readString(0);
    String currentPassword = EEPROM.readString(50);
    String deviceID = EEPROM.readString(100);

    int n = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < n; ++i) {
      options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
    }

    String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Reconfigure WiFi</title><style>";
    page += "body { font-family: Arial; text-align: center; margin-top: 50px; background-color: #f9f9f9; }";
    page += "form { display: inline-block; padding: 20px; background: white; border-radius: 10px; box-shadow: 0 0 10px #ccc; }";
    page += "input, select { padding: 10px; margin: 10px 0; width: 100%; }";
    page += "label { font-weight: bold; display: block; margin-top: 15px; }";
    page += "button { padding: 10px 20px; background-color: #27ae60; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    page += "</style></head><body>";

    page += "<h2>Reconfigure WiFi</h2>";
    page += "<form method='POST' action='/reconfigure'>";
    page += "<label>Device ID</label><input type='text' value='" + deviceID + "' readonly>";
    page += "<label>Current SSID</label><input type='text' value='" + currentSSID + "' readonly>";

    page += "<label>Select New SSID</label>";
    page += "<select name='ssid'>" + options + "</select>";

    page += "<label>Or Enter SSID</label><input type='text' name='custom_ssid' placeholder='Enter SSID manually'>";
    page += "<label>Password</label><input type='password' name='password' placeholder='Enter new password' required>";
    page += "<button type='submit'>Save & Reboot</button>";
    page += "</form></body></html>";

    server.send(200, "text/html", page);
    });

  // Get Message Endpoint (unchanged)
  server.on("/getMessage", []() {
    String deviceId = server.arg("device_id");
    String path = "/devices/" + deviceId + "/message";
    
    if (Firebase.getString(fbdo, path)) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("Hello " + deviceId);
      display.display();

      delay(2000);

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("Message " + deviceId + ": ");
      display.println(fbdo.stringData());
      display.display();
      
      String jsonResponse = "{\"message\":\"" + fbdo.stringData() + "\"}";
      server.send(200, "application/json", jsonResponse);
    } else {
      displayStatus("Read Error");
      server.send(500, "text/plain", "Error reading message");
    }
  });

  // Update Handler (modified to show message when returning to control panel)
  server.on("/update", HTTP_POST, []() {
    String deviceId = server.arg("device_id");
    String newMsg = server.arg("new_message");
    
    String path = "/devices/" + deviceId + "/message";
    
    if (Firebase.setString(fbdo, path, newMsg)) {
      // Update OLED display immediately
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("Updated (" + deviceId + ") to: ");
      display.println(newMsg);
      display.display();
      
      // Success response with JavaScript to show message when returning
      String content = String(R"=====(
      <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Update Successful</title>
        <style>
          :root {
            --primary: #4361ee;
            --success: #4cc9f0;
            --light: #f8f9fa;
          }
          body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #f5f7fa;
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
          }
          .success-panel {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.1);
            padding: 2rem;
            width: 100%;
            max-width: 500px;
            text-align: center;
          }
          .success-title {
            color: #27ae60;
            margin-bottom: 1.5rem;
          }
          .message-box {
            padding: 1rem;
            background: #f8f9fa;
            border-radius: 8px;
            margin: 1.5rem 0;
          }
          .message-text {
            font-size: 1.2em;
            margin: 0;
          }
          .back-link {
            display: inline-block;
            padding: 12px 24px;
            background: var(--primary);
            color: white;
            text-decoration: none;
            border-radius: 8px;
            margin-top: 1rem;
          }
        </style>
        <script>
          // Immediately fetch and display the message on OLED
          window.onload = function() {
            fetch('/getMessage?device_id=${DEVICE_ID}')
              .then(response => response.json())
              .then(data => {
                // This will trigger the OLED update through the existing /getMessage endpoint
              });
          };
        </script>
      </head>
      <body>
        <div class="success-panel">
          <h1 class="success-title">Update Successful!</h1>
          <p>Device ${DEVICE_ID} now displays:</p>
          <div class="message-box">
            <p class="message-text">"${NEW_MESSAGE}"</p>
          </div>
          <p>The OLED display has been updated.</p>
          <a href="/select" class="back-link">Back to Device Control</a>
        </div>
      </body>
      </html>
      )=====");
      content.replace("${DEVICE_ID}", deviceId);
      content.replace("${NEW_MESSAGE}", newMsg);
      server.send(200, "text/html", content);
    } else {
      displayStatus("Update Failed");
      server.send(500, "text/plain", "Update failed: " + fbdo.errorReason());
    }
  });

  server.onNotFound([]() {
    Serial.println("Page not found: " + server.uri());
    server.send(404, "text/plain", "404: Not Found");
  });
}

// ================= Helper Functions =================
void displayStatus(String msg) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
  delay(100);
}

void readStoredWiFi() {
  EEPROM.begin(512);
  ssid = ""; password = ""; id = "";
  
  for (int i = 0; i < 32; i++) ssid += char(EEPROM.read(i));
  for (int i = 32; i < 96; i++) password += char(EEPROM.read(i));
  for (int i = 96; i < 128; i++) id += char(EEPROM.read(i));
  
  ssid.trim();
  password.trim();
  id.trim();

  Serial.println("Read from EEPROM:");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);
  Serial.println("ID: " + id);
}

void writeData(String a, String b, String c) {
  EEPROM.begin(512);
  
  for (int i = 0; i < 32; i++) EEPROM.write(i, i < a.length() ? a[i] : 0);
  for (int i = 0; i < 64; i++) EEPROM.write(32+i, i < b.length() ? b[i] : 0);
  for (int i = 0; i < 32; i++) EEPROM.write(96+i, i < c.length() ? c[i] : 0);
  
  EEPROM.commit();
}

void setup() {
  // Hardware Initialization
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  Serial.begin(115200);
  Serial.println("\n\nDevice starting...");
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while(true);
  }
  displayStatus("ESP 32 is setting up...");

  // Main Flow
  readStoredWiFi();
  
  if (hasCredentials()) {
    if (connectWiFi()) {
      if (connectFirebase()) {
        displayFirebaseMessage();
        launchUserUI();
      } else {
        displayStatus("Firebase Error");
      }
    } else {
      startAPMode();
    }
  } else {
    startAPMode();
  }

  Serial.println("Final verification - restarting server");
  server.stop();
  server.begin();
  Serial.println("Server restarted. Ready for connections.");
}

void loop() {
  server.handleClient();
}