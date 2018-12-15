/*
  UDP Bidirectional Commmunication for server with three clients, which not all have to be present.
  No change necessary.
  For the definition of the AT commands, see for
  https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/#at-commands
  The serial monitor has to be set to Baud 19200 and CR and LF active.
  Messages to be send or received by ESP or entered via Serial Monitor have the following syntax:
  <message Msg> : <char original sender Id on send or final receiver Id on receive>
  example: 100pv:0
*/

#define DEBUG true


#include <SoftwareSerial.h>

SoftwareSerial esp8266(11, 12); // RX, TX

String me = "5";//own Id

String frId = me;//final receivers Id on receiving a message
String osId = me;//original senders Id on sending a message
long atime = millis();

bool ESPsga = true;//ESPsga (ESP self generated answer) means the incoming is supposed to be a ESP self generated answer, true for the beginning
String Msg;//net message

void setup() {
  Serial.begin(19200);
  esp8266.begin(19200);

  esp8266.setTimeout(5000);
  if (sendCom("AT+RST", "ready")) debug("RESET OK");
  else debug("RESET NOT OK");

  if (configAP()) {
    debug("AP ready");
  }
  if (configUDP()) {
    debug("UDP ready");
  }
}

void loop() {
  char InChar;
  String Sender = me;

  while (esp8266.available()) {
    if (ESPsga) {//if assumed, that it is ESP self generated answer, check it
     InChar = esp8266.read();

      if (InChar == '+') {
        esp8266.find("IPD,");
        delay(1);
        //read original sender Id: osId
        osId = (String)esp8266.parseInt();
        esp8266.find(':');
        //clear the net message: Msg
        Msg = "";
        //Found "+IPD,", so we set ESPsga false to not read it again until next message or answer
        ESPsga = false;//it is not a ESP self generated answer, so it is a message
      } else
        //clear buffer, when "+IPD," not found
        esp8266.find('\n');
    } else {//it is clear, that a message has been found
      //reads one char of the message per looping from the buffer
      InChar = esp8266.read();

      if (!(InChar == ':')) {
        Msg += InChar;//add net message together, when inChar not ':'
      } else {
        //read final receiver Id: frId
        frId = (String)esp8266.parseInt();

        //Find out who was the original sender
        switch (osId.toInt()) {
          case 0: Sender = "Computer"; break;
          case 1: Sender = "Client 1";   break;
          case 2: Sender = "Client 2";   break;
          default: Sender = "not valid sender";
        }
        //print 'Msg'
        if (frId != me)debug("Msg got: \"" + Msg + "\" from " + Sender);

        //if the message is for me, execute it, else send it to final receiver Id: frId
        if (frId == me)Serial.println("Msg for me:   \"" + Msg + "\"   from:   " + Sender);
        //when final receiver is existing, append to message Msg the original sender Id: osId and send it to final receiver
        else if (frId == "0" || frId == "1" || frId == "2") WifiSend(Msg + ":" + osId, frId);
        else debug("final receiver Id is not valid");

        //clear buffer because usable message ended
        esp8266.find('\n');
        //Message ended, ready for next receive, which is again supposed first to be a ESP self generated answer
        ESPsga  = true;
      }
    }
  } 
  while(Serial.available()) {
    //identify myself
    osId = me;
    //read message Msg

    Msg = Serial.readStringUntil(':');
    delay(1);
    //to do: parse Msg
    //when Msg does not end with "\r\n", there is more, identify the final receiver Id: frId, default it is me
    frId = me;
    //when Msg ends with "\r\n", get rid of it
    if (Msg.endsWith("\r\n")) {
      Msg = Msg.substring(0, Msg.length() - 2);
    }
    else frId = (String)(Serial.read() - '0');//read the final receiver Id: frId
    //if the message is for me execute it, else send it to final receiver Id: frId
    if (frId == me)
      debug("Msg for me: \"" + Msg + "\" from: my Serial Monitor");
    //when final receiver is existing, append to message Msg the original sender Id: osId and send it
    else if (frId == "0" || frId == "1" || frId == "2")
      WifiSend(Msg + ":" + osId, frId);
    else debug("final receiver Id is not valid");
    Serial.find('\n');//clear buffer
  } // computing and other code
}

//-----------------------------------------Config ESP8266------------------------------------

boolean configAP()
{
  boolean success = true;
  //set  AP mode (host)
  success &= (sendCom("AT+CWMODE=2", "OK"));
  //set AP to SSID "NanoESP" without password "", channel Id 5 and no encoding
  success &= (sendCom("AT+CWSAP=\"NanoESP\",\"\",5,0", "OK"));
  //Set ip addr of ESP8266 softAP
  success &= (sendCom("AT+CIPAP=\"192.168.4.1\"", "OK"));

  return success;
}

boolean configUDP()
{
  boolean success = true;
  //set transfer mode to normal transmission.
  success &= (sendCom("AT+CIPMODE=0", "OK"));
  //set to multiple connections (up to four)
  success &= (sendCom("AT+CIPMUX=1", "OK"));
  //start connection in UDP with assigned IP addresses, send channel and receive channel
  success &= sendCom("AT+CIPSTART=0,\"UDP\",\"192.168.4.2\",90,91", "OK"); //UDP Bidirectional and Broadcast to Packet sender or TWedge
  success &= sendCom("AT+CIPSTART=1,\"UDP\",\"192.168.4.3\",92,93", "OK"); //UDP Bidirectional and Broadcast to Client 1
  success &= sendCom("AT+CIPSTART=2,\"UDP\",\"192.168.4.4\",94,95", "OK"); //UDP Bidirectional and Broadcast to Client 2
  return success;
}

//-----------------------------------------------Control ESP-----------------------------------------------------

boolean sendCom(String command, char respond[])
{
  esp8266.println(command);
  if (esp8266.findUntil(respond, "ERROR"))
  {
    return true;
  }
  else
  {
    debug("ESP SEND ERROR: " + command);
    return false;
  }
}


//-------------------------------------------------Debug Functions------------------------------------------------------
void debug(String Msg)
{
  if (DEBUG)
  {
    Serial.println(Msg);
  }
}
//---------------------------------------------------------Sender--------------------------------------------------------
void WifiSend (String Msg, String Id) {
  bool sucess = sendCom("AT+CIPSEND=" + Id + "," + (String)(Msg.length()), "OK");
  delay(1);

  sucess &= sendCom(Msg, "OK");

  if (sucess)
    Serial.print("message send ok: \"" + Msg + "\"");
  switch (Id.toInt()) {
    case 0: Serial.println("   sent to Computer"); break;
    case 1: Serial.println("   sent to Client 1"); break;
    case 2: Serial.println("   sent to Client 2"); break;
    default: Serial.println("   invalid ID");
  }
}
