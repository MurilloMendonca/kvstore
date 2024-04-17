#pragma once
#include <arpa/inet.h>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <variant>
#include <vector>
#include <cstring>

typedef std::string Error;
typedef std::string Response;
typedef std::string Request;
typedef int         Socket;

class KVClient {
private:
  std::variant<Socket, Error> _sock;
  int _map_id;

public:
  KVClient(const char *ip, int port, int map_id);
  KVClient(const char *ip, int port);
  ~KVClient();

  std::optional<Response>   get(const std::string &key);
  std::optional<Error>      set(const std::string &key, const std::string &value);
  std::optional<int>        getMapId();

  std::optional<Error>      destroy();
  std::optional<Error>      destroy(int some_map_id);
  std::optional<Error>      destroyAll();
};


namespace kv::utils{
    template<typename K,typename V>
    std::optional<Error> set(KVClient &client, const K &key, const V &value){
        std::vector<char> buffer(sizeof(K));
        std::memcpy(buffer.data(), &key, sizeof(K));
        std::string key_str(buffer.begin(), buffer.end());
        buffer.resize(sizeof(V));
        std::memcpy(buffer.data(), &value, sizeof(V));
        std::string value_str(buffer.begin(), buffer.end());
        return client.set(key_str, value_str);
    }

    template<typename K,typename V>
    std::optional<V> get(KVClient &client, const K &key){
        std::vector<char> buffer(sizeof(K));
        std::memcpy(buffer.data(), &key, sizeof(K));
        std::string key_str(buffer.begin(), buffer.end());
        auto response = client.get(key_str);
        if(response.has_value()){
            if(response->size() != sizeof(V)){
                return std::nullopt;
            }
            V result;
            std::memcpy(&result, response->data(), sizeof(V));
            return result;
        }
        return std::nullopt;
    }

}
