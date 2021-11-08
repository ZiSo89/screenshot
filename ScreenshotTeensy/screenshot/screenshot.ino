#include <Arduino.h>

#define MAX_SIZE 64
int stream_count = 0;
char stream_str[MAX_SIZE];
bool stringComplete = false; // whether the string is complete
int c = 0;
char out_bytes[64];
const int split_max_ch = 64;
unsigned long last_time_send = 0;
int send_timeout = 1000;
char in_buf[64];

void setup()
{
  Serial3.begin(115200);//2000000
  pinMode(13, OUTPUT);
}

void loop()
{
  if (Serial3.available() > 0)
  {
    last_time_send = millis();
    digitalWrite(13, HIGH);
    do
    {
      // get the new byte:
      char inChar = (char)Serial3.read();
      stream_count++;
      stream_str[c] += inChar; // add it to the inputString:
      c++;
      if (c >= 63 || inChar == '\0' || inChar == '\n') // ||
      {
        c = 0;
        stringComplete = true;
      }

    } while (Serial3.available() /*&& (millis() - start_time) < (unsigned long)TIMEOUT*/);

  }

  if (stringComplete || millis() - last_time_send > send_timeout && strlen(stream_str) > 0)
  {
    //Serial3.print(stream_str);
    Serial.println(stream_str);
    int n = RawHID.send(stream_str, 0);
    Serial3.flush();
    memset(stream_str, 0, sizeof(stream_str));
    stream_count = 0;
    c = 0;
    stringComplete = false;
    digitalWrite(13, LOW);
  }

  int n = RawHID.recv(in_buf, 0); // 0 timeout = do not wait
  if (n > 0)
  {
    Serial3.print(in_buf);
    Serial.println(in_buf);
  }
}
