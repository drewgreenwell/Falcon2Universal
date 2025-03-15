class List {

#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE 200
#endif

public:
    byte length;
    byte data[MAX_LIST_SIZE];
    // pass separate input data
    void append(byte item) {
      if (length < MAX_LIST_SIZE) { 
        data[length] = item;
        length += 1;
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

    // save one list to another e.g. inputData to prevInputData
    static void saveList(List &list, List &toList){
      toList.clear();
      for (byte i = 0; i < list.length; i++){
        toList.append(list.data[i]);
      }
    }
 };
