
#include <Adafruit_GPS.h>
//#define DEBUG 1
#define MEGA
#include <Common.h>
#include <SPI.h>
#include <SD.h>
#include <IO.h>
#include <Matrix.h>

#include <Wire.h>
#include <Adafruit_LSM303_U.h>
#include <Elcano_Serial.h>
#include <FusionData.h>

#include "ros.h"
#include <std_msgs/String.h>

ros::NodeHandle nh;

std_msgs::String str_msg;
ros::Publisher Compass("compass", &str_msg);

Adafruit_LSM303_Mag mag = Adafruit_LSM303_Mag(1366123);

long CurrentHeading = -1;


long GetHeading(void)
{
  //Get a new sensor event from the magnetometer
  sensors_event_t event;
  mag.getEvent(&event);

  //Calculate the current heading (angle of the vector y,x)
  //Normalize the heading
  float heading = (atan2(event.magnetic.y, event.magnetic.x) * 180) / M_PI;

  if (heading < 0)
  {
    heading = 360 + heading;
  }
  // Converting heading to x1000
  return ((long)(heading * HEADING_PRECISION ));
}

void setup()
{
  nh.initNode();
  nh.advertise(compass);
}

void loop()
{
  str_msg.data = (String)GetHeading();
  compass.publish( &str_msg );
  nh.spinOnce();
  delay(1000);
}

}
