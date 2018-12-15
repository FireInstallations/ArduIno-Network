/*
  UDP Bidirectional Commmunication for Client 2 in a network of one server with three clients, which not all have to be present.
  No change necessary.
  For the definition of the AT commands, see for
  https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/#at-commands
  The serial monitor has to be set to Baud 19200 and CR and LF active.
  Messages to be send or received by ESP or entered via Serial Monitor have the following syntax:
  <message Msg> : <char final receiver Id on send or original sender Id on receive>
  example: 100pv:0
*/

#define DEBUG true
#define LED_WLAN 13

#include <SoftwareSerial.h>

//Ids for the devices

#define me          "2" //myself


SoftwareSerial esp8266(11, 12); // RX, TX

String frId = me;//final receivers Id on sending a message
String osId = me;//original senders Id on receiving a message
bool ESPsga = true;//ESPsga (ESP self generated answer) means the incoming is supposed to be a ESP self generated answer, true for the beginning
String Msg;//net message

void setup() {
  Serial.begin(19200);
  esp8266.begin(19200);

  esp8266.setTimeout(5000);
  if (sendCom("AT+RST", "ready")) debug("RESET OK");
  else debug("RESET NOT OK");
  esp8266.setTimeout(1000);

  if ( ConfigESP ()) debug("CONFIG OK");
  else debug("CONFIG NOT OK");

  if (ConfigWifi ()) debug("WIFI OK");
  else debug("WIFI NOT OK");
  esp8266.setTimeout(1000);

  if (StartCon()) {
    debug("Connected");
    digitalWrite(LED_WLAN, HIGH);
  }  else debug("NOT CONNECTED");

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
        //ignore the serverId
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
        Msg += InChar;//add message together, when inChar not ':'
      } else {
        //read original sender Id: osId
        osId = (String)esp8266.parseInt();

        //Find out who was the original sender
        switch (osId.toInt()) {
          case 0: Sender = "Computer"; break;
          case 1: Sender = "Client 1";   break;
          case 5: Sender = "Server";   break;
          default: Sender = "not valid sender";
        }
        //print 'Msg'
        Serial.println("Msg got: \"" + Msg + "\" from " + Sender);

        ///clear buffer because usable message ended
        esp8266.find('\n');
        //Message ended, ready for next receive, which is again supposed first to be a ESP self generated answer
        ESPsga  = true;
      }
    }
  }
  while (Serial.available()) {
    //default send to myself, final receiver, that's me
    frId = me;

    Msg = Serial.readStringUntil(':');
    delay(1);
    //to do: parse Msg
    //when Msg does not end with "\r\n", there is more, identify the final receiver Id: frId, default it is me
    //when Msg ends with "\r\n", get rid of it
    if (Msg.endsWith("\r\n")) {
      Msg = Msg.substring(0, Msg.length() - 2);
    }
    else frId = (String)(Serial.read() - '0');//read the final receiver Id: frId
    //if the message is for me execute it, else send it to the server
    if (frId == me)
      debug("Msg for me:  \"" + Msg + "\" from: my Serial Monitor");
    //when final receiver is existing, send it to the server
    else if (frId == "0" || frId == "5" || frId == "1")
      WifiSend(Msg, frId);
    else debug("final receiver Id is not valid");
    Serial.find('\n');//clear buffer
    esp8266.write(Serial.read());
  } //  computing and other code

}
//-----------------------------------------Config ESP8266------------------------------------
bool ConfigESP () {
  bool success = sendCom("AT+CIPMODE=0", "OK");
  success &= sendCom("AT+CIPMUX=1", "OK");
  //set to station (client) mode
  success &= sendCom("AT+CWMODE=1", "OK");
  //set IP addr of ESP8266 station (client)
  success &= sendCom("AT+CIPSTA=\"192.168.4.4\"", "OK");
  return success;
}

bool ConfigWifi () {
  esp8266.setTimeout(3000);
  return (sendCom("AT+CWJAP=\"NanoESP\",\"\"", "OK"));
}

bool StartCon () {
  //start connection with own Id and servers IP addr, send channel and receive channel
  return sendCom("AT+CIPSTART=1,\"UDP\",\"192.168.4.1\",95,94", "OK");

}
//-----------------------------------------------Controll ESP-----------------------------------------------------
bool sendCom(String command, char respond[])
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
  //A client can only send a message directly to the Server, so the first id is everytime the id of the connection to the server
  bool sucess = sendCom("AT+CIPSEND=1," + (String)(Msg.length() + 2), "OK");
  delay(1);

  sucess &= sendCom(Msg + ":" + Id, "OK");

  if (sucess)
    Serial.print("Msg send ok: \"" + Msg + "\"");
  switch (Id.toInt()) {
    case 0: Serial.println("   sent to Computer"); break;
    case 1: Serial.println("   sent to Client 1"); break;
    case 5: Serial.println("   sent to Server"); break;
    default: Serial.println("   invalid ID");
  }
}
