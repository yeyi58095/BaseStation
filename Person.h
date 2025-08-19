#pragma once
#include <System.hpp> // 如果你要用 UnicodeString

class Person {
public:
	Person();
	Person(const UnicodeString& name, int age);
	const UnicodeString& name() const { return name_; }
	int age() const { return age_; }
	void set_name(const UnicodeString& n) { name_ = n; }
	void set_age(int a) { age_ = a; }
private:
	UnicodeString name_;
	int age_;
};


