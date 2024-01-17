class AppTimer {
public:  
    int interval;
    bool active;
    bool repeat;
    long lastTick = -1;
    long tickCount = 0;
    bool elapsed = false;

    AppTimer(int interval, bool active, bool repeat){
      this->interval = interval;
      this->active = active;
      this->repeat = repeat;
    }

    void setup() {
    }

    void loop() {
      if (active) {
        if(elapsed && !repeat){
          return;
        }
        elapsed = false;
        long now = millis();
        if(lastTick == -1){
          lastTick = now;
        }
        if(now - lastTick > interval) { 
          elapsed = true;
          tickCount += 1;
        } else {
          elapsed = false;
        }
      }
    }

    void restart() {
      elapsed = false;
    }

    void reset() {
      lastTick = -1;
      tickCount = 0;
      elapsed = false;
    }
  };