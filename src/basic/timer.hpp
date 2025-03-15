class AppTimer {
  long lastTick = -1;
  long tickCount = 0;
  bool hasElapsed = false;
public:  
    int interval;
    bool active;
    bool repeat;
    
    AppTimer(int interval, bool active, bool repeat) {
      this->interval = interval;
      this->active = active;
      this->repeat = repeat;
    }

    long ticks() {
      return tickCount;
    }

    bool elapsed() {
      return hasElapsed;
    }

    void setup() {
    }

    void loop() {
      if (active) {
        if(hasElapsed && !repeat){
          return;
        }
        hasElapsed = false;
        long now = millis();
        if(lastTick == -1){
          lastTick = now;
        }
        if(now - lastTick > interval) { 
          lastTick = now;
          hasElapsed = true;
          tickCount += 1;
        } else {
          hasElapsed = false;
        }
      }
    }

    void restart() {
      hasElapsed = false;
    }

    void reset() {
      lastTick = -1;
      tickCount = 0;
      hasElapsed = false;
    }
  };