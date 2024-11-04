#ifndef STRUCT_H
#define STRUCT_H

#include <string>
#include <vector>
#include <iostream>
#include "serializer.h"

struct Person {
    int age;
    double height;
    std::string name;

    void serialize(std::vector<char>& buffer) const {
        Serializer::serialize(buffer, age);
        Serializer::serialize(buffer, height);
        Serializer::serialize(buffer, name);
    }

    static Person deserialize(const std::vector<char>& buffer) {
        Person person;
        person.age = Serializer::deserialize<int>(std::vector<char>(buffer.begin(), buffer.begin() + sizeof(int)));
        person.height = Serializer::deserialize<double>(std::vector<char>(buffer.begin() + sizeof(int), buffer.begin() + sizeof(int) + sizeof(double)));
        person.name = Serializer::deserialize<std::string>(std::vector<char>(buffer.begin() + sizeof(int) + sizeof(double), buffer.end()));
        return person;
    }

    friend std::ostream& operator<<(std::ostream& os, const Person& person) {
        os << "Person [Name: " << person.name << ", Age: " << person.age << ", Height: " << person.height << "]";
        return os;
    }

    short logicalSize() const {
        return name.size() + sizeof(age) + sizeof(height);
    }
};

#endif 