#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Raw readings
int16_t ax, ay, az;

// Offsets
int16_t ax_offset = 0;
int16_t ay_offset = 0;
int16_t az_offset = 0;

// Filtered values
float fax = 0, fay = 0, faz = 0;

// Previous filtered values
float prev_fax = 0, prev_fay = 0, prev_faz = 0;

// Complementary filtered values
float comp_ax = 0, comp_ay = 0, comp_az = 0;

// NEW: Additional smoothing stage
float smooth_ax = 0, smooth_ay = 0, smooth_az = 0;

// Filter parameters (made stronger for less sensitivity)
const float ALPHA_X = 0.93;  // Increased from 0.90
const float ALPHA_Y = 0.92;  // Increased from 0.86
const float ALPHA_Z = 0.95;  // Increased from 0.90

const float COMP_ALPHA_X = 0.95;  // Increased from 0.95
const float COMP_ALPHA_Y = 0.96;  // Increased from 0.88
const float COMP_ALPHA_Z = 0.98;  // Increased from 0.92

// NEW: Third smoothing stage
const float SMOOTH_ALPHA_X = 0.85;
const float SMOOTH_ALPHA_Y = 0.85;
const float SMOOTH_ALPHA_Z = 0.85;

// Deadzones (increased for less sensitivity)
const int DEADZONE_X = 8;  // Increased from 7
const int DEADZONE_Y = 8;   // Increased from 4
const int DEADZONE_Z = 10;  // Increased from 5

// NEW: Delta scaling (reduce output magnitude)
const float SCALE_X = 0.5;  // Reduce X sensitivity by 50%
const float SCALE_Y = 0.5;  // Reduce Y sensitivity by 50%
const float SCALE_Z = 0.5;  // Reduce Z sensitivity by 50%

void calibrateMPU() {
  long sumX=0, sumY=0, sumZ=0;
  Serial.println("Calibrating... Keep sensor still!");
  for(int i=0;i<500;i++){
    mpu.getAcceleration(&ax,&ay,&az);
    sumX += ax;
    sumY += ay;
    sumZ += az;
    delay(2);
  }
  ax_offset = sumX/500;
  ay_offset = sumY/500;
  az_offset = sumZ/500;
  Serial.println("Calibration done.");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21,20);
  Wire.setClock(100000);
  
  mpu.initialize();
  
  if(!mpu.testConnection()){
    Serial.println("MPU6050 connection failed!");
    while(1);
  }
  
  Serial.println("MPU6050 connected.");
  calibrateMPU();
}

void loop() {
  // Read raw
  mpu.getAcceleration(&ax,&ay,&az);
  
  // Offset
  int16_t ax_corr = ax - ax_offset;
  int16_t ay_corr = ay - ay_offset;
  int16_t az_corr = az - az_offset;
  
  // EMA (first stage)
  fax = ALPHA_X*fax + (1-ALPHA_X)*ax_corr;
  fay = ALPHA_Y*fay + (1-ALPHA_Y)*ay_corr;
  faz = ALPHA_Z*faz + (1-ALPHA_Z)*az_corr;
  
  // Complementary (second stage)
  comp_ax = COMP_ALPHA_X*comp_ax + (1-COMP_ALPHA_X)*fax;
  comp_ay = COMP_ALPHA_Y*comp_ay + (1-COMP_ALPHA_Y)*fay;
  comp_az = COMP_ALPHA_Z*comp_az + (1-COMP_ALPHA_Z)*faz;
  
  // NEW: Third smoothing stage
  smooth_ax = SMOOTH_ALPHA_X*smooth_ax + (1-SMOOTH_ALPHA_X)*comp_ax;
  smooth_ay = SMOOTH_ALPHA_Y*smooth_ay + (1-SMOOTH_ALPHA_Y)*comp_ay;
  smooth_az = SMOOTH_ALPHA_Z*smooth_az + (1-SMOOTH_ALPHA_Z)*comp_az;
  
  // Delta (using the new smoothed values)
  float dx = smooth_ax - prev_fax;
  float dy = smooth_ay - prev_fay;
  float dz = smooth_az - prev_faz;
  
  // Deadzone
  if(abs(dx)<DEADZONE_X) dx=0;
  if(abs(dy)<DEADZONE_Y) dy=0;
  if(abs(dz)<DEADZONE_Z) dz=0;
  
  // NEW: Scale down the output
  dx *= SCALE_X;
  dy *= SCALE_Y;
  dz *= SCALE_Z;
  
  // Send CSV
  Serial.print(dx,2); Serial.print(",");
  Serial.print(dy,2); Serial.print(",");
  Serial.println(dz,2);
  
  // Save previous (using smoothed values)
  prev_fax = smooth_ax;
  prev_fay = smooth_ay;
  prev_faz = smooth_az;
  
  delay(20); // 50Hz
}
