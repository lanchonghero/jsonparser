#pragma once

#include <cxxabi.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <memory>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#define MARCO_EXPAND(...)  __VA_ARGS__
#define RSEQ_N() 10,9,8,7,6,5,4,3,2,1,0
#define ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define GET_ARG_COUNT_INNER(...)  MARCO_EXPAND(ARG_N(__VA_ARGS__))
#define GET_ARG_COUNT(...)        GET_ARG_COUNT_INNER(__VA_ARGS__, RSEQ_N())

#define COMMA ,
#define GET_1(o, e)       o.e
#define GET_2(o, e, ...)  o.e COMMA GET_1(o, __VA_ARGS__)
#define GET_3(o, e, ...)  o.e COMMA GET_2(o, __VA_ARGS__)
#define GET_4(o, e, ...)  o.e COMMA GET_3(o, __VA_ARGS__)
#define GET_5(o, e, ...)  o.e COMMA GET_4(o, __VA_ARGS__)
#define GET_6(o, e, ...)  o.e COMMA GET_5(o, __VA_ARGS__)
#define GET_7(o, e, ...)  o.e COMMA GET_6(o, __VA_ARGS__)
#define GET_8(o, e, ...)  o.e COMMA GET_7(o, __VA_ARGS__)
#define GET_9(o, e, ...)  o.e COMMA GET_8(o, __VA_ARGS__)
#define GET_10(o, e, ...) o.e COMMA GET_9(o, __VA_ARGS__)

#define GET_N_(o, Count, ...) GET_ ## Count(o, __VA_ARGS__)
#define GET_N(o, Count, ...)  GET_N_(o, Count, __VA_ARGS__)
#define EXPAND_OBJECT_MEMBER(o, ...) GET_N(o, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

namespace json {

class Any;

class JsonObjectBase {
public:
  virtual ~JsonObjectBase() = default;
  virtual std::string DumpJson() = 0;
  virtual bool Parse(const rapidjson::Value& value) = 0;
  virtual void* GetObjectPtr() = 0;
  virtual void SetObject(void* obj_ptr) = 0;
  virtual JsonObjectBase* Clone() = 0;
};

template<typename T>
std::ostream& operator << (std::ostream& os, std::vector<T>& vec) {
  os << "[";
  for (int i = 0; i < vec.size(); i++) {
    os << vec[i] << ((i < vec.size() - 1) ? " " : "");
  }
  os << "]";
  return os;
}

template<typename T>
class Container {
public:
  class Inserter {
  public:
    Inserter(const std::string& name, std::shared_ptr<T> ptr) {
      Container::Instance().Set(name, ptr);
    }
  };

  static Container& Instance() {
    static Container instance;
    return instance;
  }

  std::shared_ptr<T> Get(const std::string& name) {
    auto iter = object_map_.find(name);
    if (iter == object_map_.end()) {
      return nullptr;
    }
    return iter->second;
  }

  void Set(const std::string& name, std::shared_ptr<T> ptr) {
    object_map_.emplace(name, ptr);
  }

private:
  std::unordered_map<std::string, std::shared_ptr<T>> object_map_;
};

class JsonUtil {
public:
  template<typename T>
  static std::string TypeName() {
    char* c = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, NULL);
    if (c) { return std::string(c); }
    return std::string((char*)typeid(T).name());
  }

  static std::string GetStringFromJsonValue(const rapidjson::Value &value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    std::string ret = std::string(buffer.GetString());
    return ret;
  }

  static std::vector<std::string> GetObjectMembers(const std::string& str) {
    std::vector<std::string> strs = StringSplit(str);
    StringTrim(strs);
    return strs;
  }

  static std::vector<std::string> StringSplit(const std::string& str, char sep = ',') {
    std::vector<std::string> strs;
    std::string::size_type pos1, pos2;
    pos1 = 0;
    pos2 = str.find(sep);
    while (pos2 != std::string::npos) {
      strs.push_back(str.substr(pos1, pos2 - pos1));
      pos1 = pos2 + 1;
      pos2 = str.find(sep, pos1);
    }
    if (pos1 != str.length()) {
      strs.push_back(str.substr(pos1));
    }
    return strs;
  }
  
  static void StringTrim(std::vector<std::string>& strs) {
    for (auto& str : strs)  {
      str = StringTrim(str);
    }
  }
  
  static std::string StringTrim(std::string str) {
    if (!str.empty()) {
      str.erase(0, str.find_first_not_of(" "));
      str.erase(str.find_last_not_of(" ") + 1);
    }
    if (!str.empty()) {
      str.erase(0, str.find_first_not_of("\""));
      str.erase(str.find_last_not_of("\"") + 1);
    }
    return str;
  }
};

class Any {
public:
  Any() : holder_ptr_(nullptr) {}
  
  template<typename T>
  Any(T data) : holder_ptr_(new AnyHolderImpl<T>(data)) {
  }

  Any(const Any& other) : holder_ptr_(other.holder_ptr_ ? other.holder_ptr_->Clone() : nullptr) {
  }

  template<typename T>
  Any& operator = (T value) {
    Any(value).Swap(*this);
    return *this;
  }

  void Swap(Any& other) {
    holder_ptr_.swap(other.holder_ptr_);
  }
  
  void SetData(void* data) {
    holder_ptr_->SetData(data);
  }

  template<typename T>
  T any_cast() {
    auto p = dynamic_cast<AnyHolderImpl<T>*>(holder_ptr_.get());
    return p->data_;
  }

  virtual bool Parse(const rapidjson::Value& value) {
    if (value.IsBool()) {
      Any(value.GetBool()).Swap(*this);
      return true;
    } if (value.IsInt()) {
      Any(value.GetInt()).Swap(*this);
      return true;
    } else if (value.IsUint()) {
      Any(value.GetUint()).Swap(*this);
      return true;
    } else if (value.IsInt64()) {
      Any(value.GetInt64()).Swap(*this);
      return true;
    } else if (value.IsUint64()) {
      Any(value.GetUint64()).Swap(*this);
      return true;
    } else if (value.IsString()) {
      Any(std::string(value.GetString())).Swap(*this);
      return true;
    } else if (value.IsObject()) {
      for (auto iter = value.MemberBegin(); iter != value.MemberEnd(); ++iter) {
        std::string name = iter->name.GetString();
        auto any_ptr = json::Container<Any>::Instance().Get(name);
        if (any_ptr == nullptr) {
          return false;
        }
        Any(*any_ptr).Swap(*this);

        auto obj_ptr = json::Container<json::JsonObjectBase>::Instance().Get(name);
        if (obj_ptr == nullptr) {
          return false;
        }
        obj_ptr = std::shared_ptr<json::JsonObjectBase>(obj_ptr->Clone());
        if (!obj_ptr->Parse(iter->value)) {
          return false;
        }
        SetData(obj_ptr->GetObjectPtr());
        return true;
      }
      return true;
    }
    return false;
  }

  std::string DumpJson() {
    if (holder_ptr_ != nullptr) {
      return holder_ptr_->DumpJson();
    }
    return "\"\"";
  }

private:
  class AnyHolderBase {
  public:
    virtual ~AnyHolderBase() = default;
    virtual std::string DumpOstream() = 0;
    virtual std::string DumpJson() = 0;
    virtual AnyHolderBase* Clone() = 0;
    virtual void SetData(void* data) = 0;
  };
  
  template <typename T>
  class AnyHolderImpl : public AnyHolderBase {
  public:
    AnyHolderImpl(T data) : data_(data) {
    }
  
    virtual AnyHolderBase* Clone() {
      return new AnyHolderImpl(data_);
    }
  
    virtual void SetData(void* data) {
      data_ = *(static_cast<T*>(data));
    }
  
    virtual std::string DumpOstream() {
      std::stringstream ss;
      auto type_name = json::JsonUtil::TypeName<decltype(data_)>();
      if (json::Container<json::JsonObjectBase>::Instance().Get(type_name) != nullptr) {
        ss << type_name << data_;
      } else if (std::is_base_of<decltype(data_), std::string>::value ||
          std::is_same<decltype(data_), std::string>::value ||
          std::is_same<decltype(data_), char const*>::value ||
          std::is_same<decltype(data_), char*>::value) {
        ss << "\"" << data_ << "\"";
      } else {
        ss << data_;
      }
      return ss.str();
    }

    // Real vector dump
    template<typename TI>
    std::string DumpJsonVectorImpl(std::vector<TI>& vec) {
      std::stringstream ss;
      ss << "[";
      for (int i = 0; i < vec.size(); i++) {
        ss << Any(vec[i]).DumpJson();
        if (i != vec.size() - 1) {
          ss << ",";
        }
      }
      ss << "]";
      return ss.str();
    }

    // For pass non vector type compile
    template<typename TI>
    std::string DumpJsonVectorImpl(TI& vec) {
      return "";
    }

    template<typename TI>
    struct IsVector {
      template<typename TT>
      static char func(decltype(&TT::shrink_to_fit));
      template<typename TT>
      static int func(...);
      const static bool value = (sizeof(func<TI>(NULL)) == sizeof(char));
    };

    // Real dump vector json string
    template<typename TI, typename std::enable_if<IsVector<TI>::value, int>::type = 0>
    std::string DumpJsonVector(TI& vec) {
      return DumpJsonVectorImpl(vec);
    }

    // For pass non vector type compile
    template<typename TI, typename std::enable_if<!IsVector<TI>::value, int>::type = 0>
    std::string DumpJsonVector(TI& vec) {
      return "";
    }

    virtual std::string DumpJson() {
      std::stringstream ss;
      auto type_name = json::JsonUtil::TypeName<decltype(data_)>();
      auto json_object_ptr = json::Container<json::JsonObjectBase>::Instance().Get(type_name);
      if (json_object_ptr != nullptr) {
        json_object_ptr = std::shared_ptr<json::JsonObjectBase>(json_object_ptr->Clone());
        json_object_ptr->SetObject((void*)&data_);
        ss << "{\"" << type_name << "\":" << json_object_ptr->DumpJson() << "}";
      } else if (std::is_integral<T>::value) {
        ss << data_;
      } else if (type_name.find("vector") != std::string::npos) {
        ss << DumpJsonVector(data_);
      } else {  // string
        ss << "\"" << data_ << "\"";
      }
      return ss.str();
    }
  
  public:
    T data_;
  };

public:
  std::shared_ptr<AnyHolderBase> holder_ptr_ = nullptr;
};

std::ostream& operator << (std::ostream& os, const Any& any) {
  if (any.holder_ptr_ != nullptr) {
    os << any.holder_ptr_->DumpOstream();
  }
  return os;
}

static bool ParseTo(const rapidjson::Value& value, int& to)  {
  if (!value.IsInt()) {
    return false;
  }
  to = value.GetInt();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, unsigned int& to) {
  if (!value.IsUint()) {
    return false;
  }
  to = value.GetUint();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, int64_t& to) {
  if (!value.IsInt64()) {
    return false;
  }
  to = value.GetInt64();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, uint64_t& to) {
  if (!value.IsUint64()) {
    return false;
  }
  to = value.GetUint64();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, bool& to) {
  if (!value.IsBool()) {
    return false;
  }
  to = value.GetBool();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, float& to) {
  if (!value.IsNumber()) {
    return false;
  }
  to = value.GetFloat();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, double& to) {
  if (!value.IsNumber()) {
    return false;
  }
  to = value.GetDouble();
  return true;
}

static bool ParseTo(const rapidjson::Value& value, std::string& to) {
  to = "";
  if (value.IsNull()) {
    return true;
  } else if (value.IsObject() || value.IsNumber()) {
    to = JsonUtil::GetStringFromJsonValue(value);
  } else if (!value.IsString()) {
    return false;
  } else {
    to = value.GetString();
  }
  return true;
}

template<typename T>
static bool ParseTo(const rapidjson::Value& value, std::vector<T>& to) {
  to.clear();
  if (!value.IsArray()) {
    return false;
  }
  auto array = value.GetArray();
  for (int i = 0; i < array.Size(); i++) {
    T t;
    if (!ParseTo(array[i], t)) {
      return false;
    }
    to.emplace_back(t);
  }
  return true;
}

static bool ParseTo(const rapidjson::Value& value, Any& to) {
  return to.Parse(value);
}

class JsonOutput {
public:
  template<typename T>
  static std::string DumpOstream(const std::vector<std::string>& members, int index, T value) {
    // ss << members[index] << ":" << value;
    std::stringstream ss;
    if (std::is_base_of<T, std::string>::value || std::is_same<T, std::string>::value) {
      ss << "\"" << value << "\"";
    } else {
      ss << value;
    }
    return ss.str();
  }

  template<typename T, typename... Ts>
  static std::string DumpOstream(const std::vector<std::string>& members, int index, T value, Ts... args) {
    // ss << members[index] << ":" << value << ", ";
    std::stringstream ss;
    if (std::is_base_of<T, std::string>::value || std::is_same<T, std::string>::value) {
      ss << "\"" << value << "\"" << " ";
    } else {
      ss << value << " ";
    }
    ss << DumpOstream(members, ++index, args...);
    return ss.str();
  }

  // Real vector dump
  template<typename T>
  static std::string DumpJsonVectorImpl(std::vector<T>& vec) {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < vec.size(); i++) {
      ss << DumpJson(vec[i]);
      if (i != vec.size() - 1) {
        ss << ",";
      }
    }
    ss << "]";
    return ss.str();
  }

  // For pass non vector type compile
  template<typename T>
  static std::string DumpJsonVectorImpl(T& vec) {
    return "";
  }

  template<typename T>
  struct IsVector {
    template<typename TT>
    static char func(decltype(&TT::shrink_to_fit));
    template<typename TT>
    static int func(...);
    const static bool value = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  // Real dump vector json string
  template<typename T, typename std::enable_if<IsVector<T>::value, int>::type = 0>
  static std::string DumpJsonVector(T& vec) {
    return DumpJsonVectorImpl(vec);
  }

  // For pass non vector type compile
  template<typename T, typename std::enable_if<!IsVector<T>::value, int>::type = 0>
  static std::string DumpJsonVector(T& vec) {
    return "";
  }

  template<typename T>
  struct HasDumpJson {
    template<typename TT>
    static char func(decltype(&TT::DumpJson));
    template<typename TT>
    static int func(...);
    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  // If v(type Any) has DumpJson function call it.
  template<typename T, typename std::enable_if<HasDumpJson<T>::has, int>::type = 0>
  static std::string DumpJson(T& v) {
    return v.DumpJson();
  }

  // If v do not have DumpJson function.
  template<typename T, typename std::enable_if<!HasDumpJson<T>::has, int>::type = 0>
  static std::string DumpJson(T& v) {
    std::stringstream ss;
    std::string type_name = json::JsonUtil::TypeName<T>();
    auto json_object_ptr = json::Container<json::JsonObjectBase>::Instance().Get(type_name);
    if (json_object_ptr != nullptr) {
      // Get the Object's JsonObjectBase and use it to dump json string.
      json_object_ptr = std::shared_ptr<json::JsonObjectBase>(json_object_ptr->Clone());
      json_object_ptr->SetObject((void*)&v);
      ss << json_object_ptr->DumpJson();
    } else if (type_name.find("vector") != std::string::npos) {
      // Dump vector json string.
      ss << DumpJsonVector(v);
    } else {
      // Wrapped by Any and dump json string.
      ss << Any(v).DumpJson();
    }
    return ss.str();
  }

  template<typename T>
  static void DumpJson(std::stringstream& ss, const std::vector<std::string>& members, int index, T value) {
    ss << "\"" << members[index] << "\":" << DumpJson(value);
  }

  template<typename T, typename... Ts>
  static void DumpJson(std::stringstream& ss, const std::vector<std::string>& members, int index, T value, Ts... args) {
    ss << "\"" << members[index] << "\":" << DumpJson(value) << ",";
    DumpJson(ss, members, ++index, args...);
  }
};

}  // namespace json

template<typename T>
T Parse(const std::string& json_str) {
  T object;
  rapidjson::Document doc;
  doc.Parse(json_str.c_str());
  if (doc.IsNull()) {
    return object;
  }
  std::string type_name = json::JsonUtil::TypeName<T>();
  auto json_object_ptr = json::Container<json::JsonObjectBase>::Instance().Get(type_name);
  if (json_object_ptr == nullptr) {
    return object;
  }
  json_object_ptr = std::shared_ptr<json::JsonObjectBase>(json_object_ptr->Clone());
  if (!json_object_ptr->Parse(doc)) {
    return object;
  }
  object = *static_cast<T*>(json_object_ptr->GetObjectPtr());
  return object;
}

template<typename T>
std::string Dump(const T& obj) {
  std::string type_name = json::JsonUtil::TypeName<T>();
  auto json_object_ptr = json::Container<json::JsonObjectBase>::Instance().Get(type_name);
  if (json_object_ptr == nullptr) {
    return "";
  }
  json_object_ptr = std::shared_ptr<json::JsonObjectBase>(json_object_ptr->Clone());
  json_object_ptr->SetObject((void*)&obj);
  return json_object_ptr->DumpJson();
}

#define REGISTER_JSON_OBJECT(ObjectType, ...)                                                      \
  namespace json {                                                                                 \
                                                                                                   \
  static bool ParseTo(const rapidjson::Value& value, ObjectType& to) {                             \
    std::string type_name = json::JsonUtil::TypeName<ObjectType>();                                \
    auto json_object_ptr = json::Container<json::JsonObjectBase>::Instance().Get(type_name);       \
    if (json_object_ptr == nullptr) {                                                              \
      return false;                                                                                \
    }                                                                                              \
    json_object_ptr = std::shared_ptr<json::JsonObjectBase>(json_object_ptr->Clone());             \
    if (!json_object_ptr->Parse(value)) {                                                          \
      return false;                                                                                \
    }                                                                                              \
    to = *static_cast<ObjectType*>(json_object_ptr->GetObjectPtr());                               \
    return true;                                                                                   \
  }                                                                                                \
                                                                                                   \
  static bool ParseTo(const rapidjson::Value& value, std::vector<ObjectType>& to) {                \
    to.clear();                                                                                    \
    if (!value.IsArray()) {                                                                        \
      return false;                                                                                \
    }                                                                                              \
    auto array = value.GetArray();                                                                 \
    for (int i = 0; i < array.Size(); i++) {                                                       \
      ObjectType t;                                                                                \
      if (!ParseTo(array[i], t)) {                                                                 \
        return false;                                                                              \
      }                                                                                            \
      to.emplace_back(t);                                                                          \
    }                                                                                              \
    return true;                                                                                   \
  }                                                                                                \
                                                                                                   \
  }  /* namespace json */                                                                          \
                                                                                                   \
  std::ostream& operator << (std::ostream& os, const ObjectType& object) {                         \
    std::vector<std::string> members = json::JsonUtil::GetObjectMembers(#__VA_ARGS__);             \
    auto s = json::JsonOutput::DumpOstream(members, 0, EXPAND_OBJECT_MEMBER(object, __VA_ARGS__)); \
    os << "{" << s << "}";                                                                         \
    return os;                                                                                     \
  }                                                                                                \
                                                                                                   \
  bool operator == (const ObjectType& o1, const ObjectType& o2) {                                  \
    std::string s1, s2;                                                                            \
    std::vector<std::string> members = json::JsonUtil::GetObjectMembers(#__VA_ARGS__);             \
    s1 = json::JsonOutput::DumpOstream(members, 0, EXPAND_OBJECT_MEMBER(o1, __VA_ARGS__));         \
    s2 = json::JsonOutput::DumpOstream(members, 0, EXPAND_OBJECT_MEMBER(o2, __VA_ARGS__));         \
    return s1 == s2;                                                                               \
  }                                                                                                \
                                                                                                   \
  class JsonObject##ObjectType : public json::JsonObjectBase {                                     \
  public:                                                                                          \
    JsonObject##ObjectType() {                                                                     \
    }                                                                                              \
                                                                                                   \
    JsonObject##ObjectType(ObjectType o) : object(o) {                                             \
    }                                                                                              \
                                                                                                   \
    virtual json::JsonObjectBase* Clone() {                                                        \
      return new JsonObject##ObjectType(object);                                                   \
    }                                                                                              \
                                                                                                   \
    void* GetObjectPtr() {                                                                         \
      return (void*)(&object);                                                                     \
    }                                                                                              \
                                                                                                   \
    void SetObject(void* ptr) {                                                                    \
      object = *static_cast<ObjectType*>(ptr);                                                     \
    }                                                                                              \
                                                                                                   \
    virtual std::string DumpJson() {                                                               \
      std::stringstream ss, sso;                                                                   \
      std::vector<std::string> members = json::JsonUtil::GetObjectMembers(#__VA_ARGS__);           \
      json::JsonOutput::DumpJson(sso, members, 0, EXPAND_OBJECT_MEMBER(object, __VA_ARGS__));    \
      ss << "{" << sso.str() << "}";                                                               \
      return ss.str();                                                                             \
    }                                                                                              \
                                                                                                   \
    virtual bool Parse(const rapidjson::Value& value) {                                            \
      std::vector<std::string> members = json::JsonUtil::GetObjectMembers(#__VA_ARGS__);           \
      return ParseMembers(members, 0, value, EXPAND_OBJECT_MEMBER(object, __VA_ARGS__));           \
    }                                                                                              \
                                                                                                   \
    template <typename T, typename... Args>                                                        \
    bool ParseMembers(const std::vector<std::string>& members, int index,                          \
      const rapidjson::Value& value, T& arg, Args&... args) {                                      \
        if (!ParseMembers(members, index, value, arg)) {                                           \
          return false;                                                                            \
        }                                                                                          \
        return ParseMembers(members, ++index, value, args...);                                     \
    }                                                                                              \
                                                                                                   \
    template <typename T>                                                                          \
    bool ParseMembers(const std::vector<std::string>& members, int index,                          \
      const rapidjson::Value& value, T& arg) {                                                     \
      std::string type_name = json::JsonUtil::TypeName<T>();                                       \
      if (value.IsNull()) {                                                                        \
        return true;                                                                               \
      }                                                                                            \
      const char *key = members[index].c_str();                                                    \
      if (!value.IsObject()) {                                                                     \
        return false;                                                                              \
      }                                                                                            \
      if (!value.HasMember(key)) {                                                                 \
        return true;                                                                               \
      }                                                                                            \
      if (!json::ParseTo(value[key], arg)) {                                                       \
        return false;                                                                              \
      }                                                                                            \
      return true;                                                                                 \
    }                                                                                              \
                                                                                                   \
  public:                                                                                          \
    ObjectType object;                                                                             \
  };                                                                                               \
                                                                                                   \
  json::Container<json::JsonObjectBase>::Inserter _obj_inserter_##ObjectType(                      \
    json::JsonUtil::TypeName<ObjectType>(), std::make_shared<JsonObject##ObjectType>());           \
  json::Container<json::Any>::Inserter _any_inserter_##ObjectType(                                 \
    json::JsonUtil::TypeName<ObjectType>(), std::make_shared<json::Any>(ObjectType()))
