
#include "Person.h"

Person::Person() : name_(L""), age_(0) {}
Person::Person(const UnicodeString& name, int age) : name_(name), age_(age) {}
