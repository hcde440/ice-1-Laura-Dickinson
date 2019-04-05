/*A sketch to get the ESP8266 on the network and connect to some open services via HTTP to
 * get our external IP address and (approximate) geolocative information in the getGeo()
 * function. To do this we will connect to http://freegeoip.net/json/, an endpoint which
 * requires our external IP address after the last slash in the endpoint to return location
 * data, thus http://freegeoip.net/json/XXX.XXX.XXX.XXX
 * 
 * This sketch also introduces the flexible type definition struct, which allows us to define
 * more complex data structures to make receiving larger data sets a bit cleaner/clearer.
 * 
 * jeg 2017
 * 
 * updated to new API format for Geolocation data from ipistack.com
 * brc 2019
*/
#include <ESP8266WiFi.h>            //include the library for the device
#include <ESP8266HTTPClient.h>      //include the library for the wifi connectivity component
#include <ArduinoJson.h>                                         //provides the ability to parse and construct JSON objects
const char* ssid = "fklaser";                                    //sets the wifi network you are connecting to
const char* pass = "Buddy8290!";                                 //sets the password to connect to the wifi network
const char* geoKey = "85d6995b16688f273c95691c4f40ab6c";         //sets the api key to gain access to the geolocation data
const char* weatherKey = "59dddeca5db593f3a2157dfcaf84b934";     //sets the api key to gain access to the weather data

typedef struct {      //creates a new object to hold a set of the geoloc api variables
  String ip;          //creates a string variable for the external IP address
  String cc;          //creates a string variable for country name
  String cn;          //creates a string variable for country code
  String rc;          //creates a string variable for region code
  String rn;          //creates a string variable for region name
  String cy;          //creates a string variable for city
  String ln;          //creates a string variable for longitude
  String lt;          //creates a string variable for latitude
} GeoData;            //names the new data structure of geoloc info as GeoData

typedef struct {      //creates a place to hold a set of the weather api variables
  String tp;          //creates a string variable for the temperature
  String pr;          //creates a string variable for atmospheric pressure
  String hd;          //creates a string variable for humidity
  String ws;          //creates a string variable for wind speed        
  String cd;          //creates a string variable for cloud density
} MetData;            //names the new data structure of weather info as MetData

GeoData location; //creates an instance of the GeoData set named location

MetData conditions;  //just as with the GeoData type, we need to create a variable named conditions
                     //as an instance of the MetData type

//set up serial monitor, wifi connection, and get and parse through desired api data
void setup() {
  Serial.begin(115200);                                   //sets the serial monitor speed
  delay(10);                                              //wait for 10 milliseconds
  Serial.print("Connecting to "); Serial.println(ssid);   //print the we are connecting to the wifi network
  WiFi.mode(WIFI_STA);                                    //we are connecting from an access point as a station
  WiFi.begin(ssid, pass);                                 //connect to the wifi using the id and password
  while (WiFi.status() != WL_CONNECTED) {                 //while the wifi is working but we haven't connected yet
    delay(500);                                           //wait a half second
    Serial.print(".");                                    //print a dot
  }
  
  Serial.println(); Serial.println("WiFi connected"); Serial.println();   //print the wifi is connected
  Serial.print("Your ESP has been assigned the internal IP address ");    //print the words in the quotes
  Serial.println(WiFi.localIP());                                         //append with the device's assigned ip address
  
  //get the data (parsed) for the device's current location using the geoloc api
  getGeo();                                                               
  
  Serial.println("Your external IP address is " + location.ip);                         //prints the external ip address
  Serial.print("Your ESP is currently in " + location.cn + " (" + location.cc + "),");  //prints the current country and its code
  Serial.println(" in or near " + location.cy + ", " + location.rc + ".");              //prints the current city and region
  Serial.println("and located at (roughly) ");                                          //prints the text in quotes
  Serial.println(location.lt + " latitude by " + location.ln + " longitude.");          //prints the current latitude and longitude

  //get the data (parsed) for the weather at the device's current location using the weather api
  getMet(location.cy);

  Serial.println();                                                                                                //prints a blank line
  Serial.println("With " + conditions.cd + ", the temperature in " + location.cy + ", " + location.rc);            //prints the current cloud density, city, and region 
  Serial.println("is " + conditions.tp + "F, with a humidity of " + conditions.hd + "%. The winds are blowing ");  //prints the current humidity and temperature
  Serial.println("at " + conditions.ws + " miles per hour, and the ");                                             //prints the current wind speed
  Serial.println("barometric pressure is at " + conditions.pr + " millibars.");                                    //prints the current atmospheric pressure
}

//code that executes repeatedly as the device has power
void loop() {
  //if we put getIP() here, it would ping the endpoint over and over . . . DOS attack?
}

//gets the local ip address as a string
String getIP() {
  HTTPClient theClient;                                   //creates a mini website created to get ip address
  String ipAddress;                                       //creates a string to store the ip address
  theClient.begin("http://api.ipify.org/?format=json");   //creates the initial "mini website" address
  int httpCode = theClient.GET();                         //creates and sets the http code variable
  if (httpCode > 0) {                                     //if the http code is positive
    if (httpCode == 200) {                                //if the http code is 200 (and the webiste was successfully created)
      DynamicJsonBuffer jsonBuffer;                       //use the dynamic json buffer because the mem size of data returned is unknown
      String payload = theClient.getString();             //creates a string variable called payload withn the data that contains the ip info
      JsonObject& root = jsonBuffer.parse(payload);       //make sure the string can be read as a json object
      ipAddress = root["ip"].as<String>();                //find and set the local ip address 
    } else {                                                                      //if the http code isn't positive
      Serial.println("Something went wrong with connecting to the endpoint.");    //print that the connection failed
      return "error";                                                             //return an error
    }
  }
  return ipAddress;                                       //return the device's local ip address
}

//requests and parses through the geolocation data from the geoloc api
void getGeo() {
  HTTPClient theClient;                                                               //different from old theClient, which "dies" after use
  Serial.println("Making HTTP request");                                              //print the text in the quotes
  theClient.begin("http://api.ipstack.com/" + getIP() + "?access_key=" + geoKey);     //return IP as .json object
  int httpCode = theClient.GET();                       //create a variable for and set the http code
  if (httpCode > 0) {                                   //if the http code is positive
    if (httpCode == 200) {                              //if the http code is 200 (and the webiste was successfully created)
      Serial.println("Received HTTP payload.");         //print the text in quotes
      DynamicJsonBuffer jsonBuffer;                     //use the dynamic json buffer because the mem size of data returned is unknown
      String payload = theClient.getString();           //creates a string variable called payload withn the data that contains the ip info
      Serial.println("Parsing...");                     //print the text in quotes
      JsonObject& root = jsonBuffer.parse(payload);     //make sure the string can be read as a json object
      
      // Test if parsing succeeds.
      if (!root.success()) {                            //if the parsing is unsuccessful
        Serial.println("parseObject() failed");         //print that parsing the object was unsuccessful
        return;                                         //exit the if statement and execute the rest of the code
      }

      //cast the values as Strings to match what was previously assigned
      location.ip = root["ip"].as<String>();            //set ip in the data set location as the json object labeled ip
      location.cc = root["country_code"].as<String>();  //set cc in the data set location as the json object labeled country code
      location.cn = root["country_name"].as<String>();  //set cn in the data set location as the json object labeled country name
      location.rc = root["region_code"].as<String>();   //set rc in the data set location as the json object labeled region code
      location.rn = root["region_name"].as<String>();   //set rn in the data set location as the json object labeled region name
      location.cy = root["city"].as<String>();          //set cy in the data set location as the json object labeled city
      location.lt = root["latitude"].as<String>();      //set lt in the data set location as the json object labeled latitude
      location.ln = root["longitude"].as<String>();     //set ln in the data set location as the json object labeled longitude
    } else {                                                                      //if the http code isn't positive
      Serial.println("Something went wrong with connecting to the endpoint.");    //print that something went wrong with the connection
    }
  }
}

  //requests and parses through the weather data from the weather api
  void getMet(String) {
    HTTPClient theClient;    //create a new mini website for the ip address
    //creates the address for the mini website                                                     
    theClient.begin("http://api.openweathermap.org/data/2.5/weather?q=" + location.cy + "&units=imperial&appid=" + weatherKey);
    int httpCode = theClient.GET();                               //creates and sets the http code variable
    if (httpCode > 0) {                                           //if the http code is positive
      if (httpCode == HTTP_CODE_OK) {                             //if the code is one of a successful connection
        DynamicJsonBuffer jsonBuffer;                             //use the dynamic json buffer because the mem size of data returned is unknown
        String payload = theClient.getString();                   //creates a string variable called payload withn the data that contains the ip info
        JsonObject& root = jsonBuffer.parseObject(payload);       //make sure the string can be read as a json object

        //testing if the parsing succeeds
        if (!root.success()) {                                    //if the parsing is unsuccessful
          Serial.println("parseObject() failed in getMet().");    //print that parsing the object was unsuccessful
          return;                                                 //exit the if statement and execute the rest of the code
      }

      //cast the values as Strings to match what was previously assigned
      conditions.tp = root["main"]["temp"].as<String>();                 //set tp in the data set conditions as the json object labeled temp in main
      conditions.pr = root["main"]["pressure"].as<String>();             //set pr in the data set conditions as the json object labeled pressure in main
      conditions.hd = root["main"]["humidity"].as<String>();             //set hd in the data set conditions as the json object labeled humidity in main
      conditions.cd = root["weather"][0]["description"].as<String>();    //set cd in the data set conditions as the json object labeled description in weather
      conditions.ws = root["wind"]["speed"].as<String>();                //set ws in the data set conditions as the json object labeled speed in wind
    }
  }
  //if the http code isn't positive
  else { 
    Serial.printf("Something went wrong with connecting to the endpoint in getMet().");   //print that something went wrong with the connection
  }
}
