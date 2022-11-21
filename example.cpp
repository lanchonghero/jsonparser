#include <iostream>
#include "json_parser.h"

struct Person;
REGISTER_FORWARD_DECLARATION(Person);

struct Singer {
 std::string type;
 int age;
};
REGISTER_JSON_OBJECT(Singer, type, age);

struct Address {
 std::string country;
 std::string city;
 std::string street;
 std::vector<Person> neighbors;
};
REGISTER_JSON_OBJECT(Address, country, city, street, neighbors);

struct Friend {
 std::string relation;
 json::Any secret;
};
REGISTER_JSON_OBJECT(Friend, relation, secret);

struct Person {
 std::string name;
 int age;
 Address address;
 std::vector<Friend> friends;
 json::Any secret;
};
REGISTER_JSON_OBJECT(Person, name, age, address, friends, secret);

int main(int argc, char** argv) {
  Friend f1{"my best friend", Singer{"rocker", 18}};
  Friend f2{"new friend", "little girl"};
  Friend f3{"third friend", 3};

  Person p2{"p2", 3, Address{"china", "shanghai", "putuo"}};

  Address addr1{"china", "beijing", "wangjing", {p2}};
  Person p1{"p1", 4, addr1, {f1, f2, f3}, "the kind!"};

  // // 笔试者具体实现
  auto json = Dump(p1); // 序列化
  std::cout << json << std::endl; // 打印序列化结果
  std::cout << p1 << std::endl; // 打印 Person 对象
  auto pp = Parse<Person>(json); // 反序列化 TODO: 目前Parse为函数模板，需要显示指定parse的类型，后续实现普通函数返回auto类型
  std::cout << pp << std::endl;
  assert(p1 == pp); // 反序列化的结果是对的

  return 0;
}
