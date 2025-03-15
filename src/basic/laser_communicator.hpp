class LaserCommunicator {

  #ifndef LASER_DEBUG
  #define LASER_DEBUG false
  #endif

  // boot sequence
  #define DEFAULT_BOOT_DELAY 1185       // amount of time after first message received before responding 
  #define BOOT_INTERVAL 180             // amount of time between each boot step
  #define BOOT_BUMP_AMT 32              // amount of time added to bootInterval after first message
  #define BOOT1a 4                      // send bootData1 5 times
  #define BOOT2a 1                      // send bootData2 1 time
  #define BOOT1b 5                      // send bootData1 5 times
  #define BOOT2b 1                      // send bootData2 1 time
  int boot1a = BOOT1a;
  int boot2a = BOOT2a;
  int boot1b = BOOT1b;
  int boot2b = BOOT2b;

  int txIdxVal = 0;
  // message tokens
  int lengthVal = 0;
  int rxIdxVal = 0;
  int msgTypeVal = 0;
  bool lengthMatched = false;
  bool rxIndexMatched = false;
  bool msgTypeMatched = false;
  // last byte received
  int prevData = 0;
  // flag to indicate a line of data has been received (e.g. 0x0A \n)
  bool messageReceived = false;
  // todo: store for alarms
  // List alarms {.length=0};
  // store for current and last message received
  List inputData {.length=0};
  List prevInputData {.length=0};

  // pre formatted messages for the boot sequence. after boot we are echoing the laser, monitoring alert codes, and incrementing a message index
  List bootData1 {.length=18, .data={0x49, 0x4C, 0x6D, 0x70, 0x08, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A}};
  List bootData2 {.length=14, .data={0x49, 0x4C, 0x6D, 0x70, 0x04, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0A}};
  List bootData3 {.length=10, .data={0x49, 0x4C, 0x6D, 0x70, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x0A}};

  // flags and intervals for boot
  bool rxReceived = false;              // have any complete packets have been received from laser so the boot sequence can start
  bool waitingToBoot = true;            // has the boot sequence started
  bool booted = false;                  // has the boot sequence completed
  int bootDelay = DEFAULT_BOOT_DELAY;   // number of milliseconds after input received to start boot
  int bootInterval = BOOT_INTERVAL;     // number of milliseconds between each boot message
  int lastWaitTick = 0;                 // timer to resend last message  todo: likely unnecessary. communication is fine without the extra chatter

  int lastMessageReceived = 0;          // timer to record amount of time since last message received
  int idleTimeout = 5000;               // number of milliseconds without communication from laser before resetting boot

  // HardwareSerial SerialPort;
  AppTimer bootTimer = AppTimer(DEFAULT_BOOT_DELAY, true, true);                   // timer to track bootDelay / bootInterval
  HardwareSerial *LaserSerial;

  public:
  String modelName;
  // parsed-ish version of inputData for serial debug
  String inputString;
  
  bool hasMessage() {
    return messageReceived;
  }

  bool hasBooted() {
    return booted;
  }

  bool isWaitingToBoot() {
    return waitingToBoot;
  }

  LaserCommunicator(HardwareSerial *serialPort){
    this->LaserSerial = serialPort;
    this->modelName = String("");
    this->inputString = String("");
  }

  void setup() {
    modelName.reserve(100);
    inputString.reserve(200);
  }

  void loop() {
    long now = millis();
    // check if we need to start booting
    if(!booted) {
      // once input has been recived start timing
      if(rxReceived) {
        // waiting to fire our first boot message
        if(waitingToBoot) {
          bootTimer.loop();
          if(bootTimer.elapsed()) {
            waitingToBoot = false;
            bootTimer.interval = bootInterval;
          }
        } else {
          // running boot sequence
          bootTimer.loop();
          if(bootTimer.elapsed()) {
            // 1 tick from waiting to boot, 1 tick from boot sequence = add offset on 3rd tick
            if(bootTimer.ticks() == 3) {
              // after the first boot message there is a larger offset
              // add ~32 milliseconds here to keep things lined up
              bootTimer.interval += BOOT_BUMP_AMT;
            }
            bootLoop();
          }
        }
      } else {
        // waiting for input to start boot sequence
      }
    } else {
      // no message has been revceived in enough time to assume laser is off 
      if(now - lastMessageReceived > idleTimeout) {
        resetBoot();
      }
    }
    // if we have processed a message, update it and echo back the new message after a short delay
    if(messageReceived){
      delay(30);
      handleMessage();
      messageReceived = false;
      lastWaitTick = now;
    } 
    checkSerial();
  }

  // check for communication and start parsing
  void checkSerial() {
    if(messageReceived){
      logln("skip serial check");
      return;
    }
    int bytes = 0;
    while(LaserSerial->available()){
      lastMessageReceived = millis(); 
      int data = LaserSerial->read();
      inputData.append((byte)data);
      prevData = data;
      bytes += 1;
      // processing more than one message break
      if(bytes > 7){
        if((char)data == '\n'){
          break;
        }
      }
    }
    if(bytes > 0){
      log("Input Data Length: ");
      log(bytes);
      log(" : ");
      logln(inputData.length);
      if(inputData.length > 0){
        byte lv = inputData.data[inputData.length - 1];
        char inChar = (char)lv;
        log("Last Char: ");
        log((int)lv);
        if(inChar == '\n'){
          log("New Line");
        }
        logln(inChar);
        if(inChar == '\n'){
          if(rxReceived){
            messageReceived = true;
            parse_input_data();
          } else {
            // flag that input has been received so booting can begin
            rxReceived = true;
            parse_input_data();
            log("First Input: ");
            logln(inputString);
            inputString = "";
            inputData.clear();
          }   
        }
      }
    }
  }

  void parse_input_data() {
    lengthVal = (int)inputData.data[4];
    rxIdxVal = (int)inputData.data[5];
    msgTypeVal = (int)inputData.data[6];
    #if !LASER_DEBUG
    return;
    #endif
    inputString.clear();
    inputString.concat((char)inputData.data[0]);
    inputString.concat((char)inputData.data[1]);
    inputString.concat((char)inputData.data[2]);
    inputString.concat((char)inputData.data[3]);
    inputString.concat("-");
    inputString.concat(lengthVal);
    inputString.concat("-");
    inputString.concat(rxIdxVal);
    inputString.concat("-");
    inputString.concat(msgTypeVal);
    inputString.concat("-");
    if(inputData.length > 7 + lengthVal) {
      for(int i = 7; i < inputData.length; i++) {
        if(inputData.length > 50){
          inputString.concat(String(inputData.data[i], HEX) + " ");
        } else {
          inputString.concat(String(inputData.data[i], HEX) + " ");
        }
      }
    }
    Serial.print("RX ");
    Serial.print(rxIdxVal);
    Serial.print(" Parsed String Length: ");
    Serial.println(inputData.length);
    Serial.println(inputString);
  }

  void handleMessage(){
    logln(inputString + "---" + (inputData.length) + "---" + (lengthVal));
    // dont respond with the laser header, just dump last line
    if(!booted){
      inputData.clear();
      inputString = "";
    }
    // model name is the only message at this length
    // the controller does not echo the header, it jus returns the previous message in this case
    if (inputData.length >= 100){
      byte dataLen = inputData.data[4];
      byte dataStart = 8;
      // byte end = dataStart + dataLen;
      byte chars = 20;
      byte end = dataStart + chars;
      for(int i = dataStart; i < end; i++){
        modelName.concat((char)inputData.data[i]);
      }
      modelName.trim();
      log("writing previous line instead of header: ");
      logln(modelName);
      writeListToLaser(prevInputData);
    } else {
      List::saveList(inputData, prevInputData);
      writeListToLaser(inputData);
    }
    inputData.clear();
    inputString = "";
  }

  // write the packet to serial
  void writeListToLaser(List &list){
    list.data[5] = txIdxVal;
    byte lastByte;
    for(byte i = 0; i < list.length; i++){
      lastByte = list.data[i];
      LaserSerial->write(lastByte);
      log((char)lastByte);
    }
    #if LASER_DEBUG
    // make sure debug message is closed      
    if(lastByte != 0x0A){
      Serial.println();
    }
    #endif
    txIdxVal += 1;
    if(txIdxVal > 255){
      txIdxVal = 0;
    }
    delay(30);
  }

  // restart timings and counts for boot loop
  void resetBoot() {
    logln("Resetting boot!");
    bootDelay = DEFAULT_BOOT_DELAY;
    bootInterval = BOOT_INTERVAL;
    boot1a = BOOT1a;
    boot2a = BOOT2a;
    boot1b = BOOT1b;
    boot2b = BOOT2b;
    rxReceived = false;
    booted = false;
    waitingToBoot = true;
  }

  // once communication starts, send a series of messages to handshake with the laser
  void bootLoop(){
    logln("boot loop");
    if(boot1a > 0){
      writeListToLaser(bootData1);
      boot1a -= 1;
      log("boot1a - ");
      logln(boot1a);
    } else if(boot2a > 0){
      writeListToLaser(bootData2);
      boot2a -= 1;
      log("boot2a - ");
      logln(boot2a);
    } else if(boot1b > 0){
      writeListToLaser(bootData1);
      boot1b -= 1;
      log("boot1b - ");
      logln(boot1b);
    } else if(boot2b > 0) {
      writeListToLaser(bootData3);
      boot2b -= 1;
      log("boot2b - ");
      logln(boot2b);
    } else {
      booted = true;
    }
  }

  template <typename T>
  void log(const T& data) {
    #if LASER_DEBUG
    Serial.print(data);
    #endif
  }

  template <typename T>
  void logln(const T& data) {
    #if LASER_DEBUG
    Serial.println(data);
    #endif
  }
   
};