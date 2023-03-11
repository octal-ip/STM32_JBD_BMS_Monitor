#include <Arduino.h>
#define TFT_ENABLE
#define BALANCER PB1

#ifdef TFT_ENABLE
  /* If the TFT is to be enabled:
  - Uncomment the following line in .pio\libdeps\bluepill_f103c8\TFT_eSPI\User_Setup_Select.h
      #include <User_Setups/Setup32_ILI9341_STM32F103.h>
  - And ensure the following configuration is placed in .pio\libdeps\bluepill_f103c8\TFT_eSPI\User_Setups\Setup32_ILI9341_STM32F103.h
      #define STM32
      #define ILI9341_DRIVER
      #define TFT_CS   PB13 // Chip select control pin to TFT CS
      #define TFT_DC   PB14 // Data Command control pin to TFT DC (may be labelled RS = Register Select)
      #define TFT_RST  PB15 // Reset pin to TFT RST (or RESET)
  

    TFT connections:
    CS PB13
    DC PB14
    RST PB15
    MOSI PA7
    MISO PA6
    SCK PA5
    LED +3.3v

    Additionally, the below three variables should be asjusted to suit your battery.
    This will allow the ring meters to render with the correct scale.

    Example for a 16S high current battery:
      int maxCurrent = 150;
      int minVoltage = 45;
      int maxVoltage = 58;
  */
  int maxCurrent = 30;
  int minVoltage = 17;
  int maxVoltage = 22;

  #include <SPI.h>
  #include <TFT_eSPI.h>
  #define BLUE2RED 3
  #define GREEN2RED 4
  #define RED2GREEN 5
  #define BUFF_SIZE 64
  #define TFT_GREY 0x2104 // Dark grey 16 bit colour
  TFT_eSPI tft = TFT_eSPI();
#endif

// The pin labeled RX on the bluetooth adaptor connects to PA3 (Serial2)
// The pin labeled TX on the bluetooth adaptor connects to PA2
// The data pin next to the positive wire on the BMS connects to input A of the ADUM1201.
// The data pin next to the negative wire on the BMS connects to output A of the ADUM1201
// Output B of the ADUM1201 connects to PB11 (Serial3)
// Input B of the ADUM1201 connects PB10

byte packetStatus = 0;
byte packetCount = 0;
byte incomingByte = 0;
unsigned long lastUnlock = 0;
unsigned long lastRequest = 0;
byte statSwitch = 0;
char StatString[8] = {0x0};
char tftBuff[70] = {0x0};
unsigned long bluetoothActive = 0;

float voltage = 0.0;
float current = 0.0;
float capacityRem = 0.0;
float capacityTyp = 0.0;
float cycles = 0.0;
byte protectionStatus = 0;
byte percentRem = 0;
byte NTCs = 0;
byte cells = 16; //Default to 16 cells. This will be automatically updated.
byte cellCount = 0;
float lowestCell = 0.0;
float highestCell = 0.0;
byte lowestCellNumber = 0;
byte highestCellNumber = 0;
float cellVoltageDelta = 0.0;
float temps[4] = {0.0, 0.0, 0.0, 0.0};
bool balancerOn = 0;

#ifdef TFT_ENABLE
  //Record the last updates to the TFT screen to avoid updating the screen when no change has occurred. This improves speed and reduces flickering.
  float lastCurrent = 0.0, lastVoltage = 0.0, lastHighestCell = 0.0, lastLowestCell = 0.0, lastCellVoltageDelta = 0.0, lastTemp1 = 0.0, lastTemp2 = 0.0, lastPercentRem = 0.0;
  bool lastBalancerOn = 0;
#endif

char packetbuff[100] = {0x0};

struct BMSCommands{
   const char *name;
   byte request[7];
   unsigned long LastRequest;
   unsigned long RequestFrequency;
   long value;
};

const int BMSRequestsNum = 13;

BMSCommands BMSRequests[BMSRequestsNum] = {
  //Request name, address, LastRequest, RequestFrequency, value)
  {"System_status", {0xdd, 0xa5, 0x03, 0x00, 0xff, 0xfd, 0x77}, 0, 500, 0},
  {"Battery_status", {0xdd, 0xa5, 0x04, 0x00, 0xff, 0xfc, 0x77}, 0, 2000, 0},
  {"Charge_over_temp_release", {0xdd, 0xa5, 0x19, 0x00, 0xff, 0xe7, 0x77}, 0, 60000, 0},
  {"Charge_under_temp_release", {0xdd, 0xa5, 0x1b, 0x00, 0xff, 0xe5, 0x77}, 0, 60000, 0},
  {"Discharge_over_temp_release", {0xdd, 0xa5, 0x1d, 0x00, 0xff, 0xe3, 0x77}, 0, 60000, 0},
  {"Discharge_under_temp_release", {0xdd, 0xa5, 0x1f, 0x00, 0xff, 0xe1, 0x77}, 0, 60000, 0},
  {"Batt_over_volt_trig", {0xdd, 0xa5, 0x20, 0x00, 0xff, 0xe0, 0x77}, 0, 60000, 0},
  {"Batt_over_volt_release", {0xdd, 0xa5, 0x21, 0x00, 0xff, 0xdf, 0x77}, 0, 60000, 0},
  {"Batt_under_volt_release", {0xdd, 0xa5, 0x23, 0x00, 0xff, 0xdd, 0x77}, 0, 60000, 0},
  {"Cell_over_volt_release", {0xdd, 0xa5, 0x25, 0x00, 0xff, 0xdb, 0x77}, 0, 60000, 0},
  {"Cell_under_volt_release", {0xdd, 0xa5, 0x27, 0x00, 0xff, 0xd9, 0x77}, 0, 60000, 0},
  {"Charge_over_current_trig", {0xdd, 0xa5, 0x28, 0x00, 0xff, 0xd8, 0x77}, 0, 60000, 0},
  {"Discharge_over_current_release", {0xdd, 0xa5, 0x29, 0x00, 0xff, 0xd7, 0x77}, 0, 60000, 0}
};


void setup() {
  Serial1.begin(115200); //Debug
  Serial2.begin(9600); //Bluetooth
  Serial3.begin(9600); //BMS

  #ifdef TFT_ENABLE
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Status:", 2, 170, 2);
    tft.drawString("High Cell:", 2, 185, 2);
    tft.drawString("Low Cell:", 2, 200, 2);
    tft.drawString("Delta V:", 2, 215, 2);
    tft.drawString("Power:", 180, 170, 2);
    tft.drawString("Temp1:", 180, 185, 2);
    tft.drawString("Temp2:", 180, 200, 2);
    tft.drawString("SOC:", 180, 215, 2);
    tft.drawFastHLine(0, 160, 319, TFT_WHITE);
  #endif

  pinMode(BALANCER, OUTPUT);
}

#ifdef TFT_ENABLE
  unsigned int rainbow(byte value)
  {
    // Value is expected to be in range 0-127
    // The value is converted to a spectrum colour from 0 = blue through to 127 = red

    byte red = 0; // Red is the top 5 bits of a 16 bit colour value
    byte green = 0;// Green is the middle 6 bits
    byte blue = 0; // Blue is the bottom 5 bits

    byte quadrant = value / 32;

    if (quadrant == 0) {
      blue = 31;
      green = 2 * (value % 32);
      red = 0;
    }
    if (quadrant == 1) {
      blue = 31 - (value % 32);
      green = 63;
      red = 0;
    }
    if (quadrant == 2) {
      blue = 0;
      green = 63;
      red = value % 32;
    }
    if (quadrant == 3) {
      blue = 0;
      green = 63 - 2 * (value % 32);
      red = 31;
    }
    return (red << 11) + (green << 5) + blue;
  }

  float mapf(float x, float in_min, float in_max, float out_min, float out_max)
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  int ringMeter(float value, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme)
  {
    // Minimum value of r is about 52 before value text intrudes on ring
    // drawing the text first is an option
    
    x += r; y += r;   // Calculate coords of centre of ring

    int w = r / 3;    // Width of outer ring is 1/4 of radius
    
    int angle = 150;  // Half the sweep angle of meter (300 degrees)

    int v = mapf(value, vmin, vmax, -angle, angle); // Map the value to an angle v

    byte seg = 3; // Segments are 3 degrees wide = 100 segments for 300 degrees
    byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

    // Variable to save "value" text colour from scheme and set default
    int colour = TFT_BLUE;
  
    // Draw colour blocks every inc degrees
    for (int i = -angle+inc/2; i < angle-inc/2; i += inc) {
      // Calculate pair of coordinates for segment start
      float sx = cos((i - 90) * 0.0174532925);
      float sy = sin((i - 90) * 0.0174532925);
      uint16_t x0 = sx * (r - w) + x;
      uint16_t y0 = sy * (r - w) + y;
      uint16_t x1 = sx * r + x;
      uint16_t y1 = sy * r + y;

      // Calculate pair of coordinates for segment end
      float sx2 = cos((i + seg - 90) * 0.0174532925);
      float sy2 = sin((i + seg - 90) * 0.0174532925);
      int x2 = sx2 * (r - w) + x;
      int y2 = sy2 * (r - w) + y;
      int x3 = sx2 * r + x;
      int y3 = sy2 * r + y;

      if (i < v) { // Fill in coloured segments with 2 triangles
        switch (scheme) {
          case 0: colour = TFT_RED; break; // Fixed colour
          case 1: colour = TFT_GREEN; break; // Fixed colour
          case 2: colour = TFT_BLUE; break; // Fixed colour
          case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
          case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break; // Green to red (high temperature etc)
          case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
          default: colour = TFT_BLUE; break; // Fixed colour
        }
        tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
        tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
        //text_colour = colour; // Save the last colour drawn
      }
      else // Fill in blank segments
      {
        tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
        tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
      }
    }
    // Convert value to a string
    char buf[10];
    byte len = 4; if (value > 999) len = 6;
    dtostrf(value, len, 1, buf);
    buf[len] = ' '; buf[len+1] = 0; // Add blanking space and terminator, helps to centre text too!
    // Set the text colour to default
    tft.setTextSize(1);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // Uncomment next line to set the text colour to the last segment value!
    tft.setTextColor(colour, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    // Print value, if the meter is large then use big font 8, othewise use 4
    if (r > 84) {
      tft.setTextPadding(55*4); // Allow for 4 digits each 55 pixels wide
      tft.drawString(buf, x, y, 8); // Value in middle
    }
    else {
      tft.setTextPadding(4 * 14); // Allow for 4 digits each 14 pixels wide
      tft.drawString(buf, x, y, 4); // Value in middle
    }
    tft.setTextSize(1);
    tft.setTextPadding(0);
    // Print units, if the meter is large then use big font 4, othewise use 2
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    if (r > 84) tft.drawString(units, x, y + 60, 4); // Units display
    else tft.drawString(units, x, y + 15, 2); // Units display

    // Calculate and return right hand side x coordinate
    return x + r;
  }

void updateTFT() {
    tft.setTextSize(1);
    tft.setTextPadding(82);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);

    if (current != lastCurrent || voltage != lastVoltage) {
      dtostrf(abs(current * voltage), 1, 0, StatString);
      sprintf(tftBuff, "%sW", StatString);
      tft.drawString(tftBuff, 245, 170, 2);
    }

    if (current != lastCurrent || balancerOn != lastBalancerOn) {
      ringMeter(abs(current), 0, maxCurrent, 4, 5, 76, "Amps", GREEN2RED); // Draw ring meter
      tft.setTextDatum(TL_DATUM);
      tft.setTextSize(1);
      tft.setTextPadding(100);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (current > 0 && balancerOn == 0) {
        tft.drawString("Charging", 68, 170, 2);
      }
      else if (current > 0 && balancerOn == 1) {
        tft.drawString("Charging (B)", 68, 170, 2);
      }
      else if (current < 0 && balancerOn == 0) {
        tft.drawString("Discharging", 68, 170, 2);
      }
      else if (current < 0 && balancerOn == 1) {
        tft.drawString("Discharging (B)", 68, 170, 2);
      }
      else if (current == 0 && balancerOn == 0) {
        tft.drawString("Standby", 68, 170, 2);
      }
      else if (current == 0 && balancerOn == 1) {
        tft.drawString("Standby (B)", 68, 170, 2);
      }
      lastCurrent = current;
      lastBalancerOn = balancerOn;
    }
    
    if (voltage != lastVoltage) {
      ringMeter(voltage, minVoltage, maxVoltage, 167, 5, 76, "Volts", BLUE2RED); // Draw ring meter
      lastVoltage = voltage;
    }

    tft.setTextSize(1);
    tft.setTextPadding(82);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);

    if (highestCell != lastHighestCell) {
      dtostrf(highestCell, 1, 3, StatString);
      sprintf(tftBuff, "%d: %sv", highestCellNumber, StatString);
      tft.drawString(tftBuff, 68, 185, 2);
      lastHighestCell = highestCell;
    }

    if (lowestCell != lastLowestCell) {
      dtostrf(lowestCell, 1, 3, StatString);
      sprintf(tftBuff, "%d: %sv", lowestCellNumber, StatString);
      tft.drawString(tftBuff, 68, 200, 2);
      lastLowestCell = lowestCell;
    }

    if (cellVoltageDelta != lastCellVoltageDelta) {
      dtostrf(cellVoltageDelta, 1, 3, StatString);
      sprintf(tftBuff, "%sv", StatString);
      tft.drawString(tftBuff, 68, 215, 2);
      lastCellVoltageDelta = cellVoltageDelta;
    }

    if (temps[0] != lastTemp1) {
      dtostrf(temps[0], 1, 1, StatString);
      sprintf(tftBuff, "%sc", StatString);
      tft.drawString(tftBuff, 245, 185, 2);
      lastTemp1 = temps[0];
    }
    
    if (temps[1] != lastTemp2) {
      dtostrf(temps[1], 1, 1, StatString);
      sprintf(tftBuff, "%sc", StatString);
      tft.drawString(tftBuff, 245, 200, 2);
      lastTemp2 = temps[1];
    }

    if (percentRem != lastPercentRem) {
      dtostrf(percentRem, 1, 0, StatString);
      sprintf(tftBuff, "%s%%", StatString);
      tft.drawString(tftBuff, 245, 215, 2);
      lastPercentRem = percentRem;
    }
  }
#endif

void processPacket() //This function deciphers the BMS data.
{
  int checkSumCalc = 0x0;
  int checkSumData = 0x1;
  
  for (int i = 2; i < (packetCount-3); i++) //Add all the data together, skipping the header and command code at the start (first 2 bytes), and the checksum and footer at the end (last 3 bytes).
  {
      //Serial.printf("%x ", packetbuff[i]);
      checkSumCalc += packetbuff[i];
  }
  
  checkSumCalc = 65536 - checkSumCalc;
  checkSumData = (packetbuff[packetCount-3] << 8 | packetbuff[packetCount-2]); //OR the checksum bytes provided in the data packet together to find the total.
  
  
  if (checkSumCalc == checkSumData) {
    Serial1.print("Checksum "); Serial1.print(checkSumCalc);  Serial1.println(F(" matches. "));
    //Second byte: 0x3 is BMS status information. 0x4 is battery voltages.
    if (packetbuff[1] == 0x3) {
      voltage = (packetbuff[4] << 8 | packetbuff[5])/100.0;
      Serial1.print("Pack voltage: "); Serial1.print(voltage); Serial1.println("v");

      current = 0.0;
      int current2s = packetbuff[7] | (packetbuff[6] << 8); //Combine both values.
      if (current2s >> 15 == 1) { //Find the most significant bit to determine if the number is negative (2's complement).
        current = (0 - (0x10000 - current2s))/100.0; //Calculate the negative number (2's complement).
      }
      else { //Otherwise calculate the positive number.
        current = current2s/100.0;
      }
      Serial1.print("Current: "); Serial1.println(current);

      capacityRem = (packetbuff[8] << 8 | packetbuff[9])/100.0;
      Serial1.print("Remaining capacity: "); Serial1.println(capacityRem);
      
      capacityTyp = (packetbuff[10] << 8 | packetbuff[11])/100.0;
      Serial1.print("Typical capacity: "); Serial1.println(capacityTyp);
      
      cycles = (packetbuff[12] << 8 | packetbuff[13]);
      Serial1.print("Cycles: "); Serial1.println(cycles);

      protectionStatus = (packetbuff[20] << 8 | packetbuff[21]);
      Serial1.print("Protection Status: "); Serial1.println(protectionStatus);

      percentRem = packetbuff[23];
      Serial1.print("Percent Remaining: "); Serial1.println(percentRem);
      
      cells = packetbuff[25];
      Serial1.print("Cell count: "); Serial1.println(cells);

      NTCs = packetbuff[26];
      Serial1.print("NTCs: "); Serial1.println(NTCs);

      for (int i = 0; i < NTCs; i++) {
          temps[i] = ((packetbuff[(2*i)+27] << 8 | packetbuff[(2*i)+28]) * 0.1) - 273.1;
          Serial1.print("NTC "); Serial1.print(i); Serial1.print(" temp: "); Serial1.println(temps[i]);
      }

      Serial1.println(); Serial1.println();
    }

    else if (packetbuff[1] == 0x4) {
      cellCount = (packetCount - 7)/2; // The number of cells is the size of the array in 16 bit pairs, minus header, footer and checksum.
      Serial1.print("Calculated cell count: "); Serial1.println(cellCount);
      float cellVoltages[cellCount];
      lowestCell = 10.0;
      highestCell = 0.0;
      for (int i = 0; i < cellCount; i++) {
        cellVoltages[i] = (packetbuff[(2*i)+4] << 8 | packetbuff[(2*i)+5])/1000.0;
        
        if (cellVoltages[i] < lowestCell) {
          lowestCell = cellVoltages[i];
          lowestCellNumber = i + 1;
        }
        if (cellVoltages[i] > highestCell) {
          highestCell = cellVoltages[i];
          highestCellNumber = i + 1;
        }

        Serial1.print("Cell "); Serial1.print((i+1)); Serial1.print(" voltage: "); Serial1.println(cellVoltages[i]);
        Serial1.print("Lowest cell: "); Serial1.print(lowestCellNumber); Serial1.print("  Highest cell: "); Serial1.println(highestCellNumber);
      }

      cellVoltageDelta = highestCell - lowestCell;
      Serial1.print("Cell voltage delta: "); Serial1.print(cellVoltageDelta); Serial1.println("v");
      Serial1.println(); Serial1.println();
    }

    else {
      for (int i = 0; i < BMSRequestsNum; i++) {
        if (packetbuff[1] == BMSRequests[i].request[2]) { //Check if the packet buffer contains an identifier in our lists of BMS requests.
          BMSRequests[i].value = (packetbuff[4] << 8 | packetbuff[5]);
          if (i ==2 || i == 3 || i == 4 || i == 5) { //Kelvin values for temperature must be converted to celsius.
            BMSRequests[i].value = (BMSRequests[i].value/10) - 273.15;
          }
          Serial1.print(BMSRequests[i].name); Serial1.print(": "); Serial1.println(BMSRequests[i].value);
          break;
        }
      }
    } 
  }
  else {
      Serial1.print("Checksum doesn't match. Calulated "); Serial1.print(checkSumCalc); Serial1.print(", received "); Serial1.println(checkSumData);
  }
}

void receiveData() {
  if (Serial3.available()) {
    incomingByte = Serial3.read();

    if ((unsigned long)(millis() - bluetoothActive) < 3000) {
      Serial2.write(incomingByte); //Forward all received data to the Bluetooth dongle if it's active.
    }

    //Receiving data requires a state machine to track the start of packet, packet type, reception in progress and reception completed.
    // packetStatus = 0 means we're waiting for the start of a packet to arrive.
    // packetStatus = 1 means that we've found what looks like a packet header, but need to confirm with the next byte.
    // packetStatus = 2 means we've received a data packet containing system parameters (e.g. high temperature cut-off)
    // packetStatus = 3 means we've received a system status packet
    // packetStatus = 4 means we've received a battery status packet.

    Serial1.print("Received: "); Serial1.println(incomingByte, HEX);
    if (incomingByte == 0xdd && packetStatus == 0) {
      Serial1.println("0xdd found");
      packetbuff[0] = incomingByte;
      packetCount++;
      packetStatus = 1; //Possible start of packet found.
    }
    else if (packetStatus == 1) {
      if (incomingByte >= 0x03 && incomingByte <= 0x29) {
        Serial1.print("Start of packet found: "); Serial1.println(incomingByte);
        if (incomingByte == 0x03) {
          packetStatus = 3; //Start of system status packet confirmed.
        }
        else if (incomingByte == 0x04) {
          packetStatus = 4; //Start of battery status packet confirmed.
        }
        else {
          packetStatus = 2; //Start of parameter packet confirmed.
        }
        packetbuff[packetCount] = incomingByte;
        packetCount++;
      }
      else {
        Serial3.flush();
        packetStatus = 0;
        packetCount = 0;
        Serial1.println("Not a status or parameter packet, ignoring.");
      }
    }
    else if ((packetStatus == 2 && packetCount > 8) || (packetStatus == 3 && packetCount > 42) || (packetStatus == 4 && packetCount > (2 * cells + 6))) { //If more than the expected number of bytes have been received, we must have missed the last byte of the packet. Restart.
      Serial3.flush();
      packetStatus = 0;
      packetCount = 0;
      Serial1.println(F("Too many bytes received - resetting."));
    }
    else if ((incomingByte == 0x77 && packetStatus == 2 && packetCount == 8) || (incomingByte == 0x77 && packetStatus == 3 && packetCount == 42) || (incomingByte == 0x77 && packetStatus == 4 && packetCount == (2 * cells + 6))) { //End of packet found. The expected packet length must be accurately calculated as voltage can include the packet footer 0x77, e.g. is 3447 which is 0x0d 0x77
      Serial1.print("End of packet found: "); Serial1.println(incomingByte, HEX);
      packetbuff[packetCount] = incomingByte;
      packetCount++;
      processPacket();
      packetCount = 0;
      packetStatus = 0;
      Serial3.flush();
    }
    else if (packetStatus == 2 || packetStatus == 3 || packetStatus == 4) {
      packetbuff[packetCount] = incomingByte;
      packetCount++;
    }
  }
}

void sendRequest() {
  if ((unsigned long)(millis() - lastRequest) >= 1000) { // Don't send commands to the BMS too quickly.
    if ((unsigned long)(millis() - BMSRequests[statSwitch].LastRequest) >= BMSRequests[statSwitch].RequestFrequency && packetStatus == 0) { //Only request each stat as often as we need, and don't request when a receive is already underway.
      Serial1.print("Sending: ");
      for (int j = 0; j < 7; j++) {
        Serial1.print(BMSRequests[statSwitch].request[j], HEX); Serial1.print(" ");
        Serial3.write(BMSRequests[statSwitch].request[j]);
      }
      Serial1.println();
      BMSRequests[statSwitch].LastRequest = millis();
      lastRequest = millis();
    }
    if (statSwitch < BMSRequestsNum - 1) {
      statSwitch++;
    }
    else {
      statSwitch = 0;
    }
  }
}

void loop() {
  if (Serial2.available()) {
    Serial3.write(Serial2.read()); //Forward all requests from the Bluetooth dongle to the BMS
    bluetoothActive = millis();
  }

  if ((unsigned long)(millis() - lastUnlock) >= 20000) { //This command must be sent regularly to allow querying of parameters.
    Serial3.write(0xdd); Serial3.write(0x5a); Serial3.write(0x0); Serial3.write(0x02); Serial3.write(0x56); Serial3.write(0x78); Serial3.write(0xff); Serial3.write(0x30); Serial3.write(0x77);
    lastUnlock = millis();

    if (highestCell >= 3.4 && balancerOn == 0){ //Turn the external balancer ON if the highest cell voltage is above x
      digitalWrite(BALANCER, 1);
      balancerOn = 1;
    }
    else if (highestCell < 3.35 && balancerOn == 1){ //Turn the external balancer OFF if the highest cell voltage is above
      digitalWrite(BALANCER, 0);
      balancerOn = 0;
    }
  }

  if ((unsigned long)(millis() - bluetoothActive) > 3000){ //Only try to communicate with the BMS if Bluetooth isn't active.
    sendRequest();
  }
  receiveData();
  #ifdef TFT_ENABLE
    updateTFT();
  #endif
}
