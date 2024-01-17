class List {
public:
    byte length;
    byte data[200];
    // pass separate input data
    void append(byte item) {
      if (length < 200) { 
        length += 1;
        data[length] = item;
      }
    }
    void remove(byte index) {
      if (index >= length) return;
      memmove(&data[index], &data[index+1], length - index - 1);
      length -= 1;
    }

    void clear() {
      if(length == 0) return;
      for(byte i = 0; i < length; i++){
        remove(i);
      }
      length = 0;
    }
 };
