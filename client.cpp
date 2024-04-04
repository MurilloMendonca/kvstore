#include "client.hpp"
#include <stdexcept>
#include <string>
KVClient::KVClient(const char *ip, int port, int map_id) {
  int sock = 0;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    _sock = Error("Could not create socket");
    return;
  }

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
    _sock = Error("Invalid address");
    return;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    _sock = Error("Could not connect to server");
    return;
  }

  std::string handshake = "h:" + std::to_string(map_id);
  int len = ::send(sock, handshake.c_str(), handshake.size(), 0);
  if (len == -1) {
    _sock = Error("Failed to send handshake");
    return;
  }

  char buffer[1024] = {0};
  len = read(sock, buffer, 1024);
  if (len == -1) {
    _sock = Error("Failed to read handshake");
    return;
  }

  std::string response(buffer, len);
  try {
    _map_id = std::stoi(response);
  } catch (std::invalid_argument &e) {
      printf("Failed to parse map id\n");
      printf("Response: %s\n", response.c_str());
    _sock = Error("Failed to parse map id");
    return;
  }

  _sock = Socket(sock);
}
KVClient::KVClient(const char *ip, int port) {
  int sock = 0;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    _sock = Error("Could not create socket");
    return;
  }

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
    _sock = Error("Invalid address");
    return;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    _sock = Error("Could not connect to server");
    return;
  }

  int len = ::send(sock, "h:n", 3, 0);
  if (len == -1) {
    _sock = Error("Failed to send handshake");
    return;
  }

  char buffer[1024] = {0};
  len = read(sock, buffer, 1024);
  if (len == -1) {
    _sock = Error("Failed to read handshake");
    return;
  }

  std::string response(buffer, len);
  try {
    _map_id = std::stoi(response);
  } catch (std::invalid_argument &e) {
      printf("Failed to parse map id\n");
      printf("Response: %s\n", response.c_str());
    _sock = Error("Failed to parse map id");
    return;
  }

  _sock = Socket(sock);
}

KVClient::~KVClient() {
  if (std::holds_alternative<Socket>(_sock)) {
    close(std::get<Socket>(_sock));
  }
}

std::optional<Error> KVClient::set(const std::string &key,
                                   const std::string &value) {
  if (std::holds_alternative<Error>(_sock)) {
    return std::get<Error>(_sock);
  }
  int client = std::get<Socket>(_sock);

  std::string set = "s:" + std::to_string(key.size()) + ":" + key + ":" + std::to_string(value.size()) + ":" + value + ":";

  int sentVal = ::send(client, set.c_str(), set.size(), 0);
  if (sentVal == -1) {
    return Error("Failed to send data");
  }

  char buffer[1024] = {0};

  int len = read(client, buffer, 1024);

  Response resp(buffer, len);

  if (resp.find("OK") != std::string::npos) {
    return std::nullopt;
  } else {
    return Error(resp);
  }
}

std::optional<std::string> KVClient::get(const std::string &key) {
  if (std::holds_alternative<Error>(_sock)) {
    return std::nullopt;
  }
  int client = std::get<Socket>(_sock);

  std::string get = "g:" + std::to_string(key.size()) + ":" + key + ":";

  int sentVal = ::send(client, get.c_str(), get.size(), 0);
  if (sentVal == -1) {
    return std::nullopt;
  }

  char buffer[1024] = {0};
  int len = read(client, buffer, 1024);
  if (len == -1) {
    return std::nullopt;
  }
  return std::string(buffer, len);
}

std::optional<int> KVClient::getMapId() {
  if (std::holds_alternative<Error>(_sock)) {
    return std::nullopt;
  }
  return _map_id;
}

std::optional<Error> KVClient::destroy() {
  if (std::holds_alternative<Error>(_sock)) {
    return std::get<Error>(_sock);
  }
  int client = std::get<Socket>(_sock);

  std::string destroy = "d:" + std::to_string(_map_id)+ ":";
  int sentVal = ::send(client, destroy.c_str(), destroy.size(), 0);
  if (sentVal == -1) {
    return Error("Failed to send data");
  }

  char buffer[1024] = {0};

  int len = read(client, buffer, 1024);

  Response resp(buffer, len);

  if (resp.find("OK") != std::string::npos) {
    return std::nullopt;
  } else {
    return Error(resp);
  }
}

std::optional<Error> KVClient::destroy(int some_map_id) {
  if (std::holds_alternative<Error>(_sock)) {
    return std::get<Error>(_sock);
  }
  int client = std::get<Socket>(_sock);

  std::string destroy = "d:" + std::to_string(some_map_id) + ":";

  int sentVal = ::send(client, destroy.c_str(), destroy.size(), 0);
  if (sentVal == -1) {
    return Error("Failed to send data");
  }

  char buffer[1024] = {0};

  int len = read(client, buffer, 1024);

  Response resp(buffer, len);

  if (resp.find("OK") != std::string::npos) {
    return std::nullopt;
  } else {
    return Error(resp);
  }
}

std::optional<Error> KVClient::destroyAll() {
  if (std::holds_alternative<Error>(_sock)) {
    return std::get<Error>(_sock);
  }
  int client = std::get<Socket>(_sock);

  int sentVal = ::send(client, "d:a:", 4, 0);
  if (sentVal == -1) {
    return Error("Failed to send data");
  }

  char buffer[1024] = {0};

  int len = read(client, buffer, 1024);

  Response resp(buffer, len);

  if (resp.find("OK") != std::string::npos) {
    return std::nullopt;
  } else {
    return Error(resp);
  }
}

#ifdef TEST
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <vector>
int main(int argc, char **argv) {
  int port = 8080;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }

#ifndef BENCHMARK
  int map_id = 0;

  {

    printf("Testing creating a new map\n");

    KVClient client("127.0.0.1", port);

    printf("\tSetting a value\n");
    if (auto err = client.set("hello", "world")) {
      printf("Error: %s\n", err->c_str());
      return 1;
    }

    printf("\tGetting a value\n");
    if (auto value = client.get("hello")) {
      auto val = *value;
      if (val.find("world") == std::string::npos) {
        printf("\t\tError: Expected world, got %s\n", val.c_str());
        return 1;
      }
      printf("\t\tTest passed\n");
    } else {
      printf("\tFailed to get value\n");
      return 1;
    }

    auto map = client.getMapId();
    if (!map) {
      printf("Failed to get map id\n");
      return 1;
    }
    map_id = *map;
  }

  printf("Testing using an existing map\n");
  KVClient client("127.0.0.1", port, map_id);

  printf("\tGetting a value\n");
  if (auto value = client.get("hello")) {
    auto val = *value;
    if (val.find("world") == std::string::npos) {
      printf("\t\tError: Expected world, got %s\n", val.c_str());
      return 1;
    }
    printf("\t\tTest passed\n");
  } else {
    printf("Failed to get value\n");
    return 1;
  }

  printf("\tGetting a non setted value\n");
  if (auto value = client.get("world")) {
    auto val = *value;
    if (val.find("Failed") == std::string::npos) {
      printf("\t\tError: Expected Failed, got %s\n", val.c_str());
      return 1;
    }
    printf("\t\tTest passed\n");
  } else {
    printf("\tFailed to get value\n");
    return 1;
  }

  printf("Testing custom object encoding\n");
  struct Test {
    int a;
    char b;
    float c;

    bool operator==(const Test &other) const {
      return a == other.a && b == other.b && c == other.c;
    }

    std::string encode() const {
      return std::to_string(a) + "*" + std::to_string(b) + "*" +
             std::to_string(c);
    }
    Test(const std::string &data) {
      int ib;
      sscanf(data.c_str(), "%d*%d*%f", &a, &ib, &c);
      b = static_cast<char>(ib);
    }
    Test(int a, char b, float c) : a(a), b(b), c(c) {}
    Test() {}
  };

  Test t(1, 'x', 3.14);
  auto encodedT = t.encode();
  if (auto err = client.set("testObj", encodedT)) {
    printf("Error: %s\n", err->c_str());
    return 1;
  }

  if (auto value = client.get("testObj")) {
    auto val = *value;
    Test decoded(val);
    if (t == decoded) {
      printf("\t\tTest passed\n");
    } else {
      printf("\t\tError: Expected %d %c %f, got %d %c %f\n", t.a, t.b, t.c,
             decoded.a, decoded.b, decoded.c);
      return 1;
    }
  } else {
    printf("\tFailed to get value\n");
    return 1;
  }

  printf("Testing memcopy object encoding\n");

  Test objKey(10, 'a', 3.14);
  if (auto err = kv::utils::set(client, objKey, t)) {
    printf("Error: %s\n", err->c_str());
    return 1;
  }

  if (auto value = kv::utils::get<Test,Test>(client, objKey)) {
    auto val = *value;
    if (t == val) {
      printf("\t\tTest passed\n");
    } else {
      printf("\t\tError: Expected %d %c %f, got %d %c %f\n", t.a, t.b, t.c,
             val.a, val.b, val.c);
      return 1;
    }
  } else {
    printf("\tFailed to get value\n");
    return 1;
  }

  // Destroy all maps
  client.destroyAll();
#endif
  #ifdef BENCHMARK
int N = 10000;
if (argc == 3) {
  N = std::stoi(argv[2]);
}
KVClient enduranceClient("127.0.0.1", port);

printf("Starting endurance test with %d set operations\n", N);

auto start = std::chrono::high_resolution_clock::now();

const int progressBarWidth = 50;
// Perform all set operations
for (int i = 0; i < N; ++i) {
  std::string key = "key" + std::to_string(i);
  std::string value = "value" + std::to_string(i);

  if (auto err = enduranceClient.set(key, value)) {
    printf("Set Error: %s\n", err->c_str());
    return 1;
  }

  // Update progress for set operations every 100 iterations or on the last iteration.
  if (i % 100 == 0 || i == N - 1) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeElapsed = now - start;
    double seconds = timeElapsed.count();
    double rps = (i + 1) / seconds;

    int progress = (i + 1) * 100 / N;
    int pos = (i + 1) * progressBarWidth / N;

    printf("\rSetting: [%-*s] %d%%  %.2f req/s", progressBarWidth, std::string(pos, '=').c_str(), progress, rps);
    fflush(stdout);
  }
}

printf("\nStarting endurance test with %d get operations\n", N);

// Perform all get operations
for (int i = 0; i < N; ++i) {
  std::string key = "key" + std::to_string(i);
  std::string expectedValue = "value" + std::to_string(i);

  if (auto retValue = enduranceClient.get(key)) {
    auto val = *retValue;
    if (val.find(expectedValue) == std::string::npos) {
      printf("\tError: Expected %s, got %s\n", expectedValue.c_str(), val.c_str());
      return 1;
    }
  } else {
    printf("\tFailed to get value for key %s\n", key.c_str());
    return 1;
  }

  // Update progress for get operations every 100 iterations or on the last iteration.
  if (i % 100 == 0 || i == N - 1) {
    // Progress calculation is based on the set phase completion
    int totalIterations = N + i + 1;
    int progress = totalIterations * 100 / (N * 2);
    int pos = totalIterations * progressBarWidth / (N * 2);

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeElapsed = now - start;
    double seconds = timeElapsed.count();
    double rps = totalIterations / seconds;

    printf("\rGetting: [%-*s] %d%%  %.2f req/s", progressBarWidth, std::string(pos, '=').c_str(), progress, rps);
    fflush(stdout);
  }
}

printf("\n"); // Ensure the next output starts on a new line.

auto end = std::chrono::high_resolution_clock::now();
std::chrono::duration<double> elapsed = end - start;

double seconds = elapsed.count();
double requestsPerSecond = (N * 2) / seconds; // Total requests = N sets + N gets

printf("Endurance test completed in %.2f seconds\n", seconds);
printf("Requests per second: %.2f\n", requestsPerSecond);

enduranceClient.destroy();
#endif



  return 0;
}
#endif
