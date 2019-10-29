#include <SoftwareSerial.h>;

SoftwareSerial GPRS(3, 4);   // RX, TX for the SIM800 Chip

int8_t answer;
int onModulePin= 2;
char aux_str[50];
String apiKey="XXXXYYYYXXXX"; //ussied by thingspeak 
String Feild_Num = "1"; // the Feild you want to send data to 

//////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(){
    pinMode(onModulePin, OUTPUT);
    Serial.begin(115200);
    GPRS.begin(9600);

    Serial.println("Starting...");
    power_on();
    delay(3000);
    // sets the PIN code
    sendATcommand2("AT+CPIN=0000", "OK", "ERROR", 2000);
    delay(3000);
    Serial.println("Connecting to the network...");
    Serial.println(" ");
    while( sendATcommand2("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 1000)== 0 ) Serial.print(".");
    Serial.println("GPRS is Connected to home Network");
}

//******************************************** Main **********************************************************

void loop(){
   String  Variable = "24" ; // the data i want to send to thingspeak (you can change it to your sensor Data) 
   SendData(Feild_Num , Variable);
   delay(10000); // Thingspeak requires at least 10s delay between data upload in his free version
   }

// ******************************************** Functions ****************************************************
void power_on(){

    uint8_t answer=0;

    // checks if the module is started
    answer = sendATcommand2("AT", "OK", "OK", 2000);
    if (answer == 0)
    {
        // power on pulse
        digitalWrite(onModulePin,HIGH);
        delay(3000);
        digitalWrite(onModulePin,LOW);

        // waits for an answer from the module
        while(answer == 0){     // Send AT every two seconds and wait for the answer
            answer = sendATcommand2("AT", "OK", "OK", 2000);
        }
    }

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t sendATcommand2(char* ATcommand, char* expected_answer1,
        char* expected_answer2, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, ' ', 100);    // Initialize the string

    delay(100);

    while( GPRS.available() > 0) GPRS.read();    // Clean the input buffer

    GPRS.println(ATcommand);    // Send the AT command

    x = 0;
    previous = millis();

    // this loop waits for the answer
    do{
        // if there are data in the UART input buffer, reads it and checks for the asnwer
        if(GPRS.available() != 0){
            response[x] = GPRS.read();
            x++;
            // check if the desired answer 1  is in the response of the module
            if (strstr(response, expected_answer1) != NULL)
            {
                answer = 1;
            }
            // check if the desired answer 2 is in the response of the module
            else if (strstr(response, expected_answer2) != NULL)
            {
                answer = 2;
            }
        }
    }
    // Waits for the asnwer with time out
    while((answer == 0) && ((millis() - previous) < timeout));

    return answer;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SendData(String Field , String Data){

 String getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr +="&field";
  getStr+=Field;
  getStr+="=";
  getStr += Data;
  getStr += "\r\n\r\n";

      // Selects Single-connection mode
    if (sendATcommand2("AT+CIPMUX=0", "OK", "ERROR", 1000) == 1)
    {
        // Waits for status IP INITIAL
        while(sendATcommand2("AT+CIPSTATUS", "INITIAL", "", 500)  == 0 );
        delay(5000);

        // Sets the APN, user name and password
        if (sendATcommand2("AT+CSTT= “APN”, “User Name”, “Password”", "OK",  "ERROR", 30000) == 1)
        {
            // Waits for status IP START
            while(sendATcommand2("AT+CIPSTATUS", "START", "", 500)  == 0 );
            delay(5000);

            // Brings Up Wireless Connection
            if (sendATcommand2("AT+CIICR", "OK", "ERROR", 30000) == 1)
            {
                // Waits for status IP GPRSACT
                while(sendATcommand2("AT+CIPSTATUS", "GPRSACT", "", 500)  == 0 );
                delay(5000);

                // Gets Local IP Address
                if (sendATcommand2("AT+CIFSR", ".", "ERROR", 10000) == 1)
                {
                    // Waits for status IP STATUS
                    while(sendATcommand2("AT+CIPSTATUS", "IP STATUS", "", 500)  == 0 );
                    delay(5000);
                    Serial.println("Openning TCP");

                    // Opens a TCP socket
                    if (sendATcommand2("AT+CIPSTART=\"TCP\",\"""api.thingspeak.com""\",80" ,
                            "CONNECT OK", "CONNECT FAIL", 30000) == 1)
                    {
                        Serial.println("Connected");

                        // Sends some data to the TCP socket
                        sprintf(aux_str,"AT+CIPSEND=%d", getStr.length());
                        char Sendd [80];
                        getStr.toCharArray(Sendd,getStr.length());
                        if (sendATcommand2(aux_str, ">", "ERROR", 10000) == 1)
                        {
                            sendATcommand2(Sendd, "SEND OK", "ERROR", 10000);
                        }

                        // Closes the socket
                        sendATcommand2("AT+CIPCLOSE", "CLOSE OK", "ERROR", 10000);
                    }
                    else
                    {
                        Serial.println("Error openning the connection");
                    }
                }
                else
                {
                    Serial.println("Error getting the IP address");
                }
            }
            else
            {
                Serial.println("Error bring up wireless connection");
            }
        }
        else
        {
            Serial.println("Error setting the APN");
        }
    }
    else
    {
        Serial.println("Error setting the single connection");
    }

    sendATcommand2("AT+CIPSHUT", "OK", "ERROR", 10000);

}