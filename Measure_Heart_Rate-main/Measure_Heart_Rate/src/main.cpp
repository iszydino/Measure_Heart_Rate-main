#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>
#include "Kalman.h"
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1
#define REFESHING_PERIOD_MS     10
#define BUZZER_PIN              27
#define BUZZER_CHANEL           0
#define BUZZER_FREQ             2000
#define BUZZER_RESOLUTION       8
#define AVG_HEART_RATE          (75.0)
#define GRAPH_CLEAR_TIME        1500

Kalman bpmKalman;// Khởi tạo bộ lọc Kalman
TFT_eSPI tft = TFT_eSPI();  // Khởi tạo đối tượng TFT
PulseOximeter pox;// Khởi tạo MAX30100

//Các biến 
uint32_t tsLastReport = 0;
uint32_t lastTFTRefesh = 0;
unsigned long lastUpdate = 0;
float heartRate;
uint8_t oxi;
int graphX = 2;     
const int graphBottom = 75; 
const int graphHeight = 15;        
const int graphLeft = 2;
const int graphRight = 158;
float lastHeartRate = 0;

bool beatDetectedflag = false;// cờ đánh dấu nhận được nhịp tim
bool isClearedFlag = false;// cờ đánh dấu đã clear màn hình

void drawAxis();
void drawGraph();
void clearGraph();

void onBeatDetected(){
  if (millis() - tsLastReport > REPORTING_PERIOD_MS && pox.getSpO2() != 0) 
  {
    //Cập nhật thời gian
    unsigned long now = millis();
    float dt = (now - lastUpdate) / 1000.0f;

    // Đọc giá trị O2 cảm biến
    oxi = pox.getSpO2();
    // Dùng bộ lọc Kalman Filter cho giá trị nhịp tim
    heartRate = bpmKalman.getAngle(pox.getHeartRate(), 0.0, dt);

    if(heartRate >= 110 || heartRate <= 50)// Kiểm tra nhịp tim
    {
      ledcWrite(BUZZER_CHANEL, 128);// Bật còi
    }
    else
    {
      ledcWrite(BUZZER_CHANEL, 0);// Tắt còi
    }
    tsLastReport = millis();
    beatDetectedflag = true;// bật cờ phát hiện nhịp tim
  }
}

void setup() {
  Serial.begin(115200);

  tft.setRotation(1);//Set góc quay 90 độ       
  tft.init(); //khoi tao tft               
  tft.fillScreen(TFT_BLACK); // Setup màu nền
  tft.drawRect(0, 0, 160, 128, TFT_WHITE);

  bpmKalman.setAngle(AVG_HEART_RATE);//Setup bộ lọc Kalman Filter

  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin()) {
      Serial.println("FAILED");
      for(;;);
  } else {
      Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_11MA); // Setup dòng điện cho cảm biến
  pox.setOnBeatDetectedCallback(onBeatDetected);// Gọi hàm onBeatDetected mỗi khi cảm biến nhận được nhịp tim

  //Set up còi
  ledcSetup(BUZZER_CHANEL, BUZZER_FREQ, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANEL);
}

void loop() {
  pox.update();//update cảm biến
  drawAxis();// vẽ khung đồ thị
  if(beatDetectedflag)
  {
    drawGraph();
    beatDetectedflag = false;
  }
  else if(millis() - tsLastReport > GRAPH_CLEAR_TIME)
  {
    clearGraph();
    graphX = 2;
  }
}

//Hàm vẽ khung đồ thị
void drawAxis(){
  if(millis() - lastTFTRefesh > REFESHING_PERIOD_MS)
  {
    tft.drawLine(1, 80, 158, 80, TFT_RED);
    tft.drawLine(1, 10, 1, 80, TFT_RED);
    tft.drawLine(158, 10, 158, 80,TFT_RED);
    tft.drawLine(1, 10, 158, 10,TFT_RED);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Heart", 10, 85);
    tft.drawString("Oxi", 110, 85);
    tft.drawLine(80, 80, 80, 128, TFT_RED);
    lastTFTRefesh = millis();
  }
}

void drawGraph(){
  isClearedFlag = false;

  int constrainedHR = constrain(heartRate, 20, 160);
  int y = map(constrainedHR, 20, 160, graphBottom, graphHeight);// Tính tọa độ y hiện tại
  int lastY = map(lastHeartRate, 20, 160, graphBottom, graphHeight);// tính tọa độ y trước đó
  tft.drawLine(graphX , lastY, graphX + 2, y, TFT_BLUE);// nối y trước đó với y hiện tại
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(String(heartRate), 10, 110);
  tft.drawString(String(oxi) + "%", 110, 110);
  lastHeartRate = heartRate;

  graphX += 2;
  if (graphX > graphRight)
  {
    graphX = graphLeft + 1;
    clearGraph();
  }
}

void clearGraph(){
  if(isClearedFlag)
  {
    return;
  }
  ledcWrite(BUZZER_CHANEL, 0);
  tft.fillScreen(TFT_BLACK);  
  tft.drawRect(0, 0, 160, 128, TFT_BLACK);
  isClearedFlag = true;
}


