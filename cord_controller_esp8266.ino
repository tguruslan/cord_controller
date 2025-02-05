#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

const char* ssid = "ESP_FLY";
const char* password = "12345678";

ESP8266WebServer server(80);
Servo servo;
int defaultAngle = 30;
int angle = defaultAngle;
int duration = 1; // В хвилинах
bool isServoActive = false;
unsigned long startTime = 0;
bool servoShouldReturn = false;
const int servoPin = 2;
const int buttonPin = 3;
const int ledPin = 8;
bool buttonPressed = false;

void setup() {
    servo.attach(servoPin);
    servo.write(defaultAngle);
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    
    Serial.begin(115200);
    WiFi.softAP(ssid, password);
    Serial.println("Wi-Fi AP Started");

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", getHTML());
    });
    
    server.on("/set", HTTP_POST, []() {
        if (server.hasArg("angle") && server.hasArg("time")) {
            angle = server.arg("angle").toInt() * 1.7;
            duration = server.arg("time").toInt();
            startTime = millis() + 5000; // Запуск через 5 секунд
            servoShouldReturn = true;
            server.send(200, "text/html", "<h1>start ok</h1><script>window.setTimeout(function(){window.location.href = '/';}, 1000);</script>");
        } else {
            server.send(400, "text/plain", "Invalid parameters");
        }
    });
    
    server.on("/status", HTTP_GET, []() {
        unsigned long remaining = 0;
        if (isServoActive) {
            remaining = max((unsigned long)0, (startTime - millis()) / 1000);
        } else if (servoShouldReturn) {
            remaining = max((unsigned long)0, (startTime - millis()) / 1000 + duration * 60);
        }
        server.send(200, "application/json", "{\"remaining\": " + String(remaining) + "}");
    });

    server.on("/stop", HTTP_GET, []() {
      servo.write(defaultAngle);
      digitalWrite(ledPin, HIGH);
      duration=0;
      isServoActive = false;
      server.send(200, "text/html", "<h1>stop ok</h1><script>window.setTimeout(function(){window.location.href = '/';}, 1000);</script>");
    });
    

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    
    if (digitalRead(buttonPin) == LOW && !buttonPressed) {
        buttonPressed = true;
        Serial.println("Button pressed change servo angle");
        if(isServoActive){
            servo.write(defaultAngle);
            duration=0;
            isServoActive = false;
            digitalWrite(ledPin, HIGH);
        }else{
            angle = 170;
            duration = 3;
            startTime = millis() + 5000; // Запуск через 5 секунд
            servoShouldReturn = true;
        }
    }
    
    if (digitalRead(buttonPin) == HIGH) {
        buttonPressed = false;
    }
    
    if (servoShouldReturn && millis() >= startTime) {
        smoothMoveServo(angle, 3000);
        isServoActive = true;
        startTime = millis() + duration * 60000; // Час завершення
        servoShouldReturn = false;
    }
    
    if (isServoActive && millis() >= startTime) {
        servo.write(defaultAngle);
        isServoActive = false;
        digitalWrite(ledPin, HIGH);
        Serial.println("Servo returned to default");
    }
}

void smoothMoveServo(int targetAngle, int durationMs) {
    int startAngle = servo.read();
    int steps = durationMs / 50; // Крок оновлення кожні 50 мс
    float stepSize = (targetAngle - startAngle) / (float)steps;
    digitalWrite(ledPin, LOW);
    
    for (int i = 0; i < steps; i++) {
        startAngle += stepSize;
        servo.write(startAngle);
        delay(50); 
    }
    servo.write(targetAngle);
}

String getHTML() {
    return "<!DOCTYPE html>"
            "<html lang='en'>"
            "    <head>"
            "        <meta charset='UTF-8'>"
            "        <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "        <title>Кордовий контроллер</title>"
            "        <style>"
            "            body {font-family: Arial, sans-serif;background: linear-gradient(to right, #4A90E2, #145DA0);color: white;text-align: center;margin: 0;padding: 20px;}"
            "            h2, h3 {text-shadow: 2px 2px 5px rgba(0, 0, 0, 0.3);}"
            "            form {background: rgba(255, 255, 255, 0.1);padding: 20px;border-radius: 10px;display: inline-block;box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.3);}"
            "            input[type='range'] {width: 80%;margin: 10px 0;}"
            "            input[type='submit'] {background: #FFD700;border: none;padding: 10px 20px;font-size: 16px;cursor: pointer;border-radius: 5px;transition: 0.3s;}"
            "            input[type='submit']:hover {background: #FFC107;}"
            "            a {display: inline-block;margin-top: 10px;text-decoration: none;background: red;color: white;padding: 10px 20px;border-radius: 5px;transition: 0.3s;}"
            "            a:hover {background: darkred;}"
            "        </style>"
            "    </head>"
            "    <body>"
            "        <h2>Кордовий контроллер</h2>"
            "        <form action='/set' method='post'>"
            "            Оберти (0-100 %): <input type='range' name='angle' min='0' max='100' step='10' value='80' required oninput='this.nextElementSibling.value = this.value'><output>80</output><br>"
            "            Час (1-10 хв): <input type='range' name='time' min='1' max='10' value='3' required oninput='this.nextElementSibling.value = this.value'><output>3</output><br>"
            "            <input type='submit' value='Застосувати'><br>"
            "            <a href='/stop'>Стоп</a>"
            "        </form>"
            "        <h3>Залишилось часу: <span id='remaining'>0</span> секунд</h3>"
            "        <script>"
            "            function updateStatus() {"
            "                fetch('/status').then(response => response.json()).then(data => {"
            "                    document.getElementById('remaining').innerText = data.remaining;"
            "                });"
            "            }"
            "            setInterval(updateStatus, 1000);"
            "        </script>"
            "    </body>"
            "</html>";
}
