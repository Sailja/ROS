// getting speed from wheel on ROS environment PUBLISHER

#include "ros.h"
#include <std_msgs/String.h>

#define LOOP_TIME_MS 1000
#define CLICK_TIME_MS 1000
#define WHEEL_DIAMETER_MM 397
#define MEG 1000000
#ifndef PI
#define PI ((float) 3.1415925)
#endif
#define MAX_SPEED_KPH 50  
#define MAX_SPEED_mmPs   ((MAX_SPEED_KPH * MEG) / 3600)
unsigned long MinTickTime_ms;
#define MIN_SPEED_mPh 500
unsigned long MaxTickTime_ms;
float Odometer_m = 0;
long SpeedCyclometer_mmPs = 0;
// Speed in revolutions per second is independent of wheel size.
float SpeedCyclometer_revPs = 0.0;//revolutions per sec
volatile unsigned long TickTime = 0;
long WheelRev_ms = 0;
unsigned long OldTick = 0;
#define IRQ_NONE 0
#define IRQ_FIRST 1
#define IRQ_RUNNING 2
volatile int InterruptState = IRQ_NONE;
unsigned long ShowTime_ms;


ros::NodeHandle nh;
std_msgs::String str_msg;
ros::Publisher speedometer("speedometer", &str_msg);

void WheelRev()
{
    static int flip = 0;
    unsigned long tick;   
    noInterrupts();
    tick = millis();
    if (InterruptState != IRQ_RUNNING)
    // Need to process 1st two interrupts before results are meaningful.
        InterruptState++;

    if (tick - TickTime > MinTickTime_ms)
    {
        OldTick = TickTime;
        TickTime = tick;
    }
    if (flip)
        digitalWrite(13, LOW);
    else
        digitalWrite(13, HIGH);
    flip =!flip;  
    
    interrupts();
}
void show_speed()
{
    ShowTime_ms = millis();	
   if (InterruptState == IRQ_NONE || InterruptState == IRQ_FIRST)  // no OR 1 interrupts
   {
       SpeedCyclometer_mmPs = 0;
       SpeedCyclometer_revPs = 0;
   } 
  // check if velocity has gone to zero
//  Serial.print (" Times: Show: ");
//  Serial.print (ShowTime_ms);
//  Serial.print (" OldTick: ");
//  Serial.print (OldTick);
//  Serial.print (", Tick: ");
//  Serial.println (TickTime);

    if(ShowTime_ms - TickTime > MaxTickTime_ms)
    {  // stopped
        SpeedCyclometer_mmPs = 0;
        SpeedCyclometer_revPs = 0;
    }
    else
    {  // moving
        WheelRev_ms = max(TickTime - OldTick, ShowTime_ms - TickTime);
        if (InterruptState == IRQ_RUNNING)
        {  // have new data
      
            float Circum_mm = (WHEEL_DIAMETER_MM * PI);
            if (WheelRev_ms > 0)
            {
                SpeedCyclometer_revPs = 1000.0 / WheelRev_ms;
                SpeedCyclometer_mmPs  = Circum_mm * SpeedCyclometer_revPs;
            }            else
            {
                SpeedCyclometer_mmPs = 0;
                SpeedCyclometer_revPs = 0;
            }
        }
//      Serial.print (" revolutions = ");
//      Serial.print (revolutions);
//      Serial.print (" state = ");
//      Serial.print (InterruptState);
    }
  
    unsigned long WheelRev_s, WheelRevFraction_s;  
//  Serial.print("; WheelRev_ms = ");
//  Serial.println (WheelRev_ms);
//  Serial.print(" ms ");
//  Serial.print("speed: ");
    Serial.print(SpeedCyclometer_mmPs/1000);
    Serial.print(".");    
    Serial.print(SpeedCyclometer_mmPs%1000);
    Serial.print(" m/s; ");
    Serial.print(SpeedCyclometer_revPs);
    Serial.print(" Rev/s");
//  Serial.print(" last interval at: ");
//  unsigned long Interval_ms = ShowTime_ms - TickTime;
//  WheelRev_s = Interval_ms / 1000;
//  WheelRevFraction_s = Interval_ms % 1000;
//  Serial.print (WheelRev_s);
//  Serial.print(".");
//  Serial.print (WheelRevFraction_s);
//  Serial.print(" s ");
// Serial.print("; ");
    Serial.print (SpeedCyclometer_mmPs*3600.0/MEG);
    Serial.print(" km/h, ");
    Odometer_m += (float)(LOOP_TIME_MS * SpeedCyclometer_mmPs) / MEG;
    Serial.print(" distance traveled: ");
    Serial.print (Odometer_m);
    Serial.println(" m "); 
    char output[100] = String(SpeedCyclometer_mmPs*3600.0/MEG) + " km/h";
    str_msg.data = output;
    speedometer.publish(&str_msg);
}


void setup() 
{ 
	nh.initNode();
	nh.advertise(speedometer);

    Serial.begin(9600);//serial monitor

    pinMode(13, OUTPUT); //led
    digitalWrite(13, HIGH);//turn LED on
    
    pinMode(2, INPUT);//pulls input HIGH
    float MinTick = WHEEL_DIAMETER_MM * PI;
//    Serial.print (" MinTick = ");
//    Serial.println (MinTick);
    MinTick *= 1000.0;
    MinTick /= MAX_SPEED_mmPs;
//    Serial.print (MinTick);
    MinTickTime_ms = MinTick;
//    Serial.print (" MinTickTime_ms = ");
//    Serial.println (MinTickTime_ms);

//    Serial.print (" MIN_SPEED_mPh = ");
//    Serial.print (MIN_SPEED_mPh);
    float MIN_SPEED_mmPs =  ((MIN_SPEED_mPh * 1000.0) / 3600.0);
    // MIN_SPEED_mmPs = 135 mm/s
//    Serial.print (" MIN_SPEED_mmPs = ");
//    Serial.print (MIN_SPEED_mmPs);
    float MaxTick = (WHEEL_DIAMETER_MM * PI * 1000.0) / MIN_SPEED_mmPs;
//    Serial.print (" MaxTick = ");
//    Serial.print (MaxTick);
    MaxTickTime_ms = MaxTick;
//    Serial.print (" MaxTickTime = ");
//    Serial.println (MaxTickTime_ms);
    TickTime = millis();
    // OldTick will normally be less than TickTime.
    // When it is greater, TickTime - OldTick is a large positive number,
    // indicating that we have not moved.
    // TickTime would overflow after days of continuous operation, causing a glitch of
    // a display of zero speed.  It is unlikely that we have enough battery power to ever see this.
    OldTick = TickTime;
    ShowTime_ms = TickTime;
    WheelRev_ms = 0;
    InterruptState = IRQ_NONE;
#ifdef CLICK_IN
    attachInterrupt (0, WheelRev, RISING);//pin 2 on Mega
#endif
    Serial.println("setup complete");
}

void loop() 
{
    int i, cycles;
    unsigned long time, endTime;
    time = millis();
    if (LOOP_TIME_MS > CLICK_TIME_MS)
    {  // high speed
         cycles = LOOP_TIME_MS / CLICK_TIME_MS;
         // delay until endTime 
         // keeps a constant rate of loop calls, but don't count time in loop
        for (i=0; i<cycles; i++)
        {
            endTime = time + CLICK_TIME_MS ;//loop at 0.25 sec
            while (time < endTime)
            {
                time = millis();
            }
#ifndef CLICK_IN      
           WheelRev();
#endif
        }
        show_speed();
    }
    else  // low speed
    {
        cycles = CLICK_TIME_MS / LOOP_TIME_MS;
        for (i=0; i<cycles; i++)
        {
            endTime = time + LOOP_TIME_MS ;
            while (time < endTime)
            {
                time = millis();
            }
            show_speed();
        }
#ifndef CLICK_IN      
        WheelRev();
        show_speed();
#endif
    }
nh.spinOnce();
delay(1000);
}




