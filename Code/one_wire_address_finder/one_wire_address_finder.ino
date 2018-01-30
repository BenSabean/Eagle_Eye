#include <OneWire.h>
// Onewire lib is here http://www.pjrc.com/teensy/td_libs_OneWire.html

OneWire  ds(8);  // on pin 8 here, don't forget 4.7K resistor between +5V and DQ pin

void setup(void)
{
  Serial.begin(9600);
}

void loop(void)
{
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr))
  {
    Serial.println("END OF SCAN.");
    Serial.println();

    ds.reset_search();
    delay(2500);
    return;
  }

  Serial.println("--------------");

  Serial.print(" ROM =");
  for ( i = 0; i < 8; i++) {
    Serial.write(' ');
    if ( addr[i] < 16) {
      Serial.print("0");
    }
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();

  Serial.print("CHIP FAMILY ");
  Serial.print(addr[0], HEX);
  // the first ROM byte indicates which chip
  Serial.print(" =  ");


}


