//Exemplo subscribe - particle reference

SerialLogHandler logHandler;
int i = 0;

void myHandler(const char *event, const char *data)
{
  i++;
  Log.info("%d: event=%s data=%s", i, event, (data ? data : "NULL"));
}

void setup()
{
  Particle.subscribe("temperature", myHandler);
}

// To use Particle.subscribe(), define a handler function and register it, typically in setup().
// You can register a method in a C++ object as a subscription handler.

#include "Particle.h"

SerialLogHandler logHandler;

class MyClass {
public:
    MyClass();
    virtual ~MyClass();

    void setup();

    void subscriptionHandler(const char *eventName, const char *data);
};

MyClass::MyClass() {
}

MyClass::~MyClass() {
}

void MyClass::setup() {
    Particle.subscribe("myEvent", &MyClass::subscriptionHandler, this);
}

void MyClass::subscriptionHandler(const char *eventName, const char *data) {
    Log.info("eventName=%s data=%s", eventName, data);
}

// In this example, MyClass is a globally constructed object.
MyClass myClass;

void setup() {
    myClass.setup();
}

void loop() {

}