#include "mapper.h"
#include <arpa/inet.h>
#include <functional>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <variant>
#define BUFFER_SIZE 1024
typedef std::string Error;
typedef std::string Response;


struct GetRequest {
  std::string key;
};

struct SetRequest {
  std::string key;
  std::string value;
};

struct DestroyRequest {
  int map_id;
};

typedef std::variant<GetRequest, SetRequest, DestroyRequest, Error>
    RequestOrError;

std::variant<int, Error> readInt(std::function<std::optional<char>()> getChar) {
  std::string str;
  auto c = getChar();
  if (!c)
    return Error("Invalid request format, expected integer, got nothing");
  while (c != ':') {
    str += *c;
    c = getChar();
    if (!c)
      return Error("Invalid request format, expected integer, got nothing");
  }
  try {
    return std::stoi(str);
  } catch (std::invalid_argument &e) {
    return Error("Invalid request format, expected integer, got " + str);
  }
}

std::optional<std::string>
readString(int len, std::function<std::optional<char>()> getChar) {
  std::string str;
  for (int i = 0; i < len; i++) {
    auto c = getChar();
    if (!c)
      return std::nullopt;
    str += *c;
  }
  return str;
}
std::tuple<SetRequest, Error>
parseSetRequest(std::function<std::optional<char>()> getCharFromSocket) {
  auto keyLen = readInt(getCharFromSocket);
  if (std::holds_alternative<Error>(keyLen)) {
    return std::make_tuple(SetRequest{}, std::get<Error>(keyLen));
  }
  /* printf("[SERVER_INFO]Key length: %d\n", std::get<int>(keyLen)); */
  auto key = *readString(std::get<int>(keyLen), getCharFromSocket);

  /* printf("[SERVER_INFO]Key: %*s\n",(int)key.size(), key.c_str()); */

  auto c = getCharFromSocket();
  if (!c)
    return std::make_tuple(SetRequest{}, "Invalid request format, expected ':', and got nothing");
  if (*c != ':')
    return std::make_tuple(SetRequest{}, "Invalid request format, expected ':', and got " + std::string(1, *c));

  auto valueLen = readInt(getCharFromSocket);
  if (std::holds_alternative<Error>(valueLen)) {
    return std::make_tuple(SetRequest{}, std::get<Error>(valueLen));
  }
  /* printf("[SERVER_INFO]Value length: %d\n", std::get<int>(valueLen)); */

  auto value = *readString(std::get<int>(valueLen), getCharFromSocket);

  /* printf("[SERVER_INFO]Value: %*s\n",(int)value.size(), value.c_str()); */

  c = getCharFromSocket();
  if (!c)
    return std::make_tuple(SetRequest{}, "Invalid request format, expected ':', and got nothing");
  if (*c != ':')
    return std::make_tuple(SetRequest{}, "Invalid request format, expected ':', and got " + std::string(1, *c));

  return std::make_tuple(SetRequest{key, value}, "");
}

std::tuple<GetRequest, Error>
parseGetRequest(std::function<std::optional<char>()> getCharFromSocket) {
    auto keyLen = readInt(getCharFromSocket);
    if (std::holds_alternative<Error>(keyLen)) {
        return std::make_tuple(GetRequest{}, std::get<Error>(keyLen));
    }
    auto key = *readString(std::get<int>(keyLen), getCharFromSocket);
    
    auto c = getCharFromSocket();
    if (!c || *c != ':')
        return std::make_tuple(GetRequest{}, "Invalid request format, expected ':'");
    
    return std::make_tuple(GetRequest{key}, "");
}

// <d>:<a\map_id>: - destroy request
// <a> - all maps
std::tuple<DestroyRequest, Error>
parseDestroyRequest(std::function<std::optional<char>()> getCharFromSocket) {
  auto c = getCharFromSocket();
  if (!c)
    return std::make_tuple(DestroyRequest{}, "Invalid request format, expected 'a' or map_id, got nothing");
  if (*c == 'a') {
    c = getCharFromSocket();
    if (!c || *c != ':')
      return std::make_tuple(DestroyRequest{}, "Invalid request format, expected ':'");
    return std::make_tuple(DestroyRequest{-1}, "");
  }
  std::string map_id;
  while (*c != ':') {
    map_id += *c;
    c = getCharFromSocket();
    if (!c)
      return std::make_tuple(DestroyRequest{}, "Invalid request format, expected ':', got nothing");
  }
  try {
    return std::make_tuple(DestroyRequest{std::stoi(map_id)}, "");
  } catch (std::invalid_argument &e) {
    return std::make_tuple(DestroyRequest{}, "Invalid request format, expected integer, got " + map_id);
  }

}

// <g>:<keyLen>:<key>: - get request
// <s>:<keyLen>:<key>:<valueLen>:<value>: - set request
// <h>:<n\map_id>: - handshake request
// <d>:<a\map_id>: - destroy request
RequestOrError parseRequest(int socket) {
  char buffer[BUFFER_SIZE] = {0};
  int bufferIndex = 0;
  int bufferLength = 0;
  auto getCharFromSocket = [&socket, &buffer, &bufferIndex,
                            &bufferLength]() -> std::optional<char> {
    if (bufferIndex == bufferLength) {
      bufferLength = read(socket, buffer, BUFFER_SIZE);
      bufferIndex = 0;
      if (bufferLength == 0) {
        return std::nullopt;
      }
    }
    return buffer[bufferIndex++];
  };
  auto c = getCharFromSocket();
  if (!c)
    return Error("Client Disconnected");
  if (getCharFromSocket() != ':') {
    return Error("Invalid request format, expected ':'");
  }

  switch (*c) {
  case 'g': {
    auto [req, err] = parseGetRequest(getCharFromSocket);
    if (err != "") {
      return err;
    }
    return req;
  }
  case 's': {
    auto [req, err] = parseSetRequest(getCharFromSocket);
    if (err != "") {
      return err;
    }
    return req;
  }
  case 'd': {
    auto [req, err] = parseDestroyRequest(getCharFromSocket);
    if (err != "") {
      return err;
    }
    return req;
  }

  default:
    return Error("Invalid request format");
  }
}

Response processTask(RequestOrError task, int map_id) {
  if (std::holds_alternative<GetRequest>(task)) {
    auto getRequest = std::get<GetRequest>(task);
#ifdef DEBUG
    printf("[SERVER_INFO]Get request: %s\n", getRequest.key.c_str());
#endif
    char buffer[2 << 20] = {0};
    int len =
        get_val(map_id, getRequest.key.c_str(), getRequest.key.size(), buffer);
    if (len == -1) {
      return Error("Failed");
    }
    auto resp = std::string(buffer, len);
#ifdef DEBUG
    printf("[SERVER_INFO]Get response: %*s\n", (int)resp.size(), resp.c_str());
#endif
    return resp;
  } else if (std::holds_alternative<SetRequest>(task)) {
    auto setRequest = std::get<SetRequest>(task);
#ifdef DEBUG
    printf("[SERVER_INFO]Set request: \n\tKey=%s "
           "\n\tVal=-%*s-\n\tKey_len=%zu\n\tVal_len=%zu\n",
           setRequest.key.c_str(), (int)setRequest.value.size(),
           setRequest.value.c_str(), setRequest.key.size(),
           setRequest.value.size());
#endif
    int resp = set_val(map_id, setRequest.key.c_str(), setRequest.key.size(),
                       setRequest.value.c_str(), setRequest.value.size());
    return resp == -1 ? "Failed" : "OK";
  } else if (std::holds_alternative<DestroyRequest>(task)) {
    auto destroyRequest = std::get<DestroyRequest>(task);
    if (destroyRequest.map_id == -1) {
      destroy_all_maps();
      return "OK";
    } else {
      destroy_map(destroyRequest.map_id);
      return "OK";
    }
  }

  else {
    return std::get<Error>(task);
  }
}

bool sendResult(int clientSocket, std::string result) {
  int bytesSent = send(clientSocket, result.c_str(), result.size(), 0);
  return bytesSent >= 0;
}

// Handshake protocol is = h:<n\map_id>
// n means client wants to create a new map
// map_id means client wants to use an existing map
int receiveHandshake(int socket) {
  char buffer[1024] = {0};
  int map_id = -1;
  int valread = read(socket, buffer, 1024);
  if (valread == 0) {
    return -1;
  }
  std::string request(buffer);
  if (request[0] != 'h') {
    return -1;
  }
  if (request[2] == 'n') {
    printf("[SERVER_INFO]New map created\n");
    map_id = init_map();
  } else {
    map_id = std::stoi(request.substr(2));
  }

  int len = send(socket, std::to_string(map_id).c_str(),
                 std::to_string(map_id).size(), 0);
  if (len == -1) {
    return -1;
  }

  return map_id;
}
void handleClient(int socket) {
  int map_id = receiveHandshake(socket);
  printf("[SERVER_INFO]\tMap id: %d\n", map_id);
  while (map_id != -1) {

    auto task = parseRequest(socket);

    if (std::holds_alternative<Error>(task)) {
      printf("[SERVER_ERROR]%s\n", std::get<Error>(task).c_str());
      sendResult(socket, std::get<Error>(task).c_str());
      break;
    }

    Response resp = processTask(task, map_id);

    bool status = sendResult(socket, resp);

    if (!status) {
      printf("[SERVER_ERROR]Failed to send result\n");
      break;
    }
  }
  printf("[SERVER_INFO]Client disconnected\n");
  close(socket);
}

int main(int argc, char const *argv[]) {

  int port = 8080;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }

  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0) {
    printf("[SERVER_INFO]Error opening socket");
    return 1;
  }
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(port);
  if (bind(serverSocket, (struct sockaddr *)&serverAddress,
           sizeof(serverAddress)) < 0) {
    printf("[SERVER_ERROR]Error binding socket");
    return 1;
  }

  printf("[SERVER_INFO]Server started\n");
  printf("[SERVER_INFO]Listening on port %d\n", port);

  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  listen(serverSocket, 5);
  while (true) {
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress,
                              &clientAddressLength);
    if (clientSocket < 0) {
      printf("[SERVER_ERROR]Error accepting client");
      return 1;
    }
    printf("[SERVER_INFO]New client connected\n");
    std::thread(handleClient, clientSocket).detach();
  }
}
