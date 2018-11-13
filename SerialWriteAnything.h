#include <Arduino.h>  // for type definitions

template <class T> int Serial_writeAnything(const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          Serial.write(*p++);
    return i;
}

template <class T> int Serial_readAnything(T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = Serial.read();
    return i;
}