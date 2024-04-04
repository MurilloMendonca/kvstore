## The Redis situation
As of March 20, 2024, Redis is [no longer open source](https://redis.com/blog/redis-adopts-dual-source-available-licensing/). This is a big deal for the community, as Redis has been a popular choice for caching and other use cases. The company behind Redis, Redis Labs, has decided to change the license of Redis to the Redis Source Available License (RSAL). This means that the source code of Redis is still available, but it is no longer open source.

While that is controversial in itself, the real problem comes from the services that offer Redis as a managed service. These services, such as Amazon ElastiCache, Google Cloud Memorystore, and Microsoft Azure Cache for Redis, are now in a difficult situation. They can no longer offer Redis as a managed service without violating the terms of the RSAL. This has led to a lot of uncertainty in the community, as many users rely on these services for their Redis deployments.

For me, that highlights the importance and advantages of a deep understanding of the software stack you are using. Even your most trusted dependencies, still have the potential to change in ways that can have a big impact on your business and projects. Dependencies should be treated as a risk, something your code depends on, and you choose to depend on for the practicality it brings. But you should always be aware of the risks and have a plan in place in case things go south.

So, one should ask yourself:

- 1: What does "Dependency X" provide me?
- 2: How much control does "Dependency X" have over my project?
- 3: Could I replace "Dependency X" with something else if needed?
- 4: Could I maintain "Dependency X" myself if needed?
- 5: Can I rewrite "Dependency X" from scratch?

For questions 1 and 2, I was able to quickly find and answer: "Redis is basically a key-value store, and my code usually just interacts with it setting and getting values. Wait, Redis is just a Hashmap as a service!!". And for hell with questions 3 and 4, I was dying to answer 5.

## C to the rescue

Now, with the quest to implement the simplest key-value store system, I could not think of a better language than C. C is the language of choice when it comes to systems programming, and I personally love it for its simplicity, power and easy of interfacing with any other language. I would not write a complete server in C, but that is not the goal, C is a library language, and I used it as such.

The first thing designed was the API, and I wanted to keep it as simple as possible. The API should be able to create a new map, set a value in the map, get a value from the map, and destroy the map. The API should be able to handle multiple maps, and access them using an ID. So, I came up with the name "mapper" and the following API:
```c
#ifndef MAPPER_H
#define MAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// Inits a new map
// Returns the id
// Returns -1 if it fails
int init_map();

// Set the value of a key in the map
// Returns 0 if it succeeds
// Returns -1 if it fails
// If the key already exists, it will OVERWRITE the value
int set_val(int map_id, const char* key, int key_len, const char* value, int value_len);

// Get the value of a key in the map
// Returns the length of the value
// Returns -1 if it fails
int get_val(int map_id, const char* key, int key_len, char* value);

// Destroy a map
// free the memory of the key-value pairs
// And make the map_id invalid
// Does NOT revaildate the map_id (It can NOT be initialized again)
void destroy_map(int map_id);

// Destroy all maps
// free the memory of the key-value pairs
// And make the all map_id invalid
// DOES revaildate all the map_id (They can be initialized again)
void destroy_all_maps();

#ifdef __cplusplus
}
#endif

#endif // MAPPER_H
```

It is not perfect in any way, but it is simple and gets the job done. It may be a little troublesome to keep passing lengths of strings, but it is a small price to pay for the simplicity of the API and to not deal with strings in C. Also, it allows for binary data to be stored, which is a requirement for a key-value store with a Redis-like use case.

The last design decisions on the mapper library, such as the data structures to use were made on the fly, not perfect again, but good enough for the purpose of the project. The data structure used was a simple linked list, as it is simple to implement and works well for small data sets. The key-value pairs are stored in a linked list, and the maps are stored in an array of linked lists. The map_id is the index of the array where the linked list is stored.

```c
#define MAX_KEY_SIZE 2048
#define MAX_VALUE_SIZE 10<<20
#define HASH_MAP_SIZE 101
#define NUMBER_OF_MAPS 100
typedef char *value;
typedef const char *key;
typedef struct entry entry;

struct entry {
  key key;
  int key_len;
  value value;
  int value_len;
  entry *next;
};
typedef entry **hash_map_t;

hash_map_t maps[NUMBER_OF_MAPS];
int maps_count = 0;
```

I could have used some hash map library, maybe something like a simple header-only library, but I wanted to keep the project as simple as possible, and I wanted to have full control over the code. The linked list implementation is simple and works well for small data sets, which is what I was aiming for. And remember, the goal was to prove that it could be done, not to create a production-ready key-value store.

The implementation of the API is also simple, and I will not go into details here, as the code is available on GitHub, and it is no fun, just the usual C stuff, fulfilling the API contract. The only other code section worth mentioning is a simple set of tests that I wrote to test the API. The tests are located in the same .c file as the API implementation, just set apart by a #ifdef TESTS directive. The tests are simple and just test the basic functionality of the API, but they are enough to prove that the API works as expected.

```c
#ifdef TEST
int main() {
  int my_map = init_map();
  set_val(my_map, "key1", 4, "value1", 6);
  set_val(my_map, "key2", 4, "value2", 6);
  set_val(my_map, "key3", 4, "value3", 6);
  set_val(my_map, "key3", 4, "value9", 6);

  char buffer[100];
  int len = get_val(my_map, "key1", 4, buffer);
  printf("key1: %*s\n", len, buffer);
  len = get_val(my_map, "key2", 4, buffer);
  printf("key2: %*s\n", len, buffer);
  len = get_val(my_map, "key3", 4, buffer);
  printf("key3: %*s\n", len, buffer);
  destroy_map(my_map);

  return 0;
}
#endif
```

Is that the recommended way to write tests? No, but it is simple and works for the purpose of the project. And again, that is the goal here, I don't want to write some nonsense google test and compile it with CMake, I just want to test the API and see if it works. And indeed, after running the tests with sanitizers, correcting some memory leaks and bugs, the API worked as expected.

## The Server

But Redis is not a library, it is a server, and I wanted to have a server too. So, I decided to implement a simple server that would listen for connections on a TCP port and handle requests to the key-value store. The server would spawn a new thread for each connection, and the thread would handle the request and send the response back to the client. The server would use the mapper library to interact with the key-value store.

What better to interact with some C API than C++? I don't think that these words are often said, but I wanted to use C++ for the server. It is the language I have the more experience with and also, I like to explore new ways to write some modern C++ code, but avoiding the overuse of OOP design patterns. So, a bunch of optionals, lambdas, variants and tuples after, I had a simple server that could handle requests through a simple protocol over TCP.

```cpp
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

typedef std::variant<GetRequest, SetRequest, DestroyRequest, Error> RequestOrError;

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
```

## The Client

But a server is not useful without a client, I needed some kind of client to interact with the server. I could have written a simple client in any language, the task of the client is only to send data to the server following the protocol. So, for a turn, I decided to write a C++ library for the client, using the same modern C++ features I used for the server. Unlike the server, the client is written as a library, and it is not meant to be run as a standalone program, but to be used by other programs. And also, unlike the mapper library, the client library is meant to be used by some other user that is not me, so I tried to make it as simple and expressive on the API, using the modern C++ features.

```cpp
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
    std::optional<Error> set(KVClient &client, const K &key, const V &value);

    template<typename K,typename V>
    std::optional<V> get(KVClient &client, const K &key);
}
```

This KVClient class is the interface between the user and the Key-Value server, just like a Redis client would be. The user can create a new client, set and get values, destroy the map, and destroy all maps. Unlike the C API, I think that the expressiveness of the C++ API allows for the discard of comments, as the API is self-explanatory. The client is also simple, and it is just a wrapper around the socket API, sending and receiving data from the server. The designed use is for strings, but utils functions are provided to allow for any type to be used, as long as it can be trivially flattened to a string.

Similarly to the Mapper lib, the client is also tested in the same .cpp file as the implementation, just set apart by a #ifdef TESTS directive. The tests are simple and just test the basic functionality of the API, but they are enough to prove that the API works as expected. Also, a benchmark was written to test the performance of the server, and it was able to handle thousands of requests per second, which is more than enough for most use cases.

```
./clientTest
Testing creating a new map
    Setting a value
    Getting a value
        Test passed
Testing using an existing map
    Getting a value
        Test passed
    Getting a non setted value
        Test passed
Testing custom object encoding
    Test passed
Testing memcopy object encoding
    Test passed
```

```
./benchmark 8080 10000
Starting endurance test with 10000 set operations
Setting: [==================================================] 100%  16029.90 req/s
Starting endurance test with 10000 get operations
Getting: [==================================================] 100%  10659.79 req/s
Endurance test completed in 1.88 seconds
Requests per second: 10659.70
```

So, the benchmark shows that the server is able to handle about 10k requests per second (small string requests), which is more than enough for most use cases. But, it is good to remember that the underlying map is just a fixed size array with a bunch of linked lists, that means that this does not scale well with the number of keys, and the server will start to slow down as the number of keys increases. This can be shown by the following benchmark, that shows the performance of the server as the number of keys increases.

```
./benchmark 8080 100000
Starting endurance test with 100000 set operations
Setting: [==================================================] 100%  13943.74 req/s
Starting endurance test with 100000 get operations
Getting: [==================================================] 100%  9378.88 req/s
Endurance test completed in 21.32 seconds
Requests per second: 9378.87

./benchmark 8080 1000000
Starting endurance test with 1000000 set operations
Setting: [==================================================] 100%  2943.22 req/s
Starting endurance test with 1000000 get operations
Getting: [==================================================] 100%  2683.49 req/s
Endurance test completed in 745.30 seconds
Requests per second: 2683.49
```

As the number of keys increases, the performance of the server decreases, and it is not able to handle as many requests per second. And the decrease is not exactly linear, because the lists are not just getting long, but also fragmented on the memory, leading to more cache misses and slower access times. This could be improved using maybe a dynamic array on the hash map, this way the memory would be more contiguous, and some sort of binary search could be used to find the key-value pair, but that would be a lot of work, so I decided to leave it as it is.


## Actually using it

Now, with the server tested and benchmarked, I wanted to test it in a real-world scenario. So I took my last project that used Redis, my very own PNG CDN, that stores PNG images on disk, scale down and quantize the colors to make a smaller version, intended for caching. I could not use my C++ client, as the project was in Go, but instead of writing a whole new client in Go, I decided to use the net package to interact with the server. Where I used to call the Redis client with a get or set command, I now send a request as a string to the port where the server is listening. The server then processes the request and sends back the response, which is then parsed by the Go code.

```go
func addImageToCache(w http.ResponseWriter, fileName string, client net.Conn) {
    f, err := os.Open(fileName)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }
    defer f.Close()

    buf := make([]byte, 1<<20)
    val,_ := f.Read(buf)

    encodedString := base64.StdEncoding.EncodeToString(buf[:val])
    setTask := fmt.Sprintf("s:%d:%s:%d:%s:", len(fileName), fileName, len(encodedString), encodedString)
    client.Write([]byte(setTask))

    _, err = client.Read(buf)

    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    if !strings.Contains(string(buf), "OK") {
        http.Error(w, "Failed to cache image", http.StatusInternalServerError)
        return
    }
}

```

This did not work at first, when I was trying to send the image on the original encoding, my tests show that the server is able to handle it, but there was something going wrong when trying to format this values in Go, so I decided to encode the image in base64, and it worked. The server was able to handle the request and send back the response, and the Go code was able to parse the response and handle it accordingly. And my example project running on top of this logic worked as if it was using Redis, nothing to change.

## Conclusion

So, in the end, I was able to prove that it is possible to implement a simple key-value store, that does what your project needs, in a few days, maybe weeks to be more reliable and add some kind of authentication. More than that, it was relatively easy to implement in C and C++, some languages that are not known for their ease of use. The server was able to handle thousands of requests per second, which is more than enough for most use cases, and the client was able to interact with the server in a simple and expressive way, at least for my opinion.

Do not be blindly dependent on your dependencies, they are just code, and as a programmer, you should be able to write something at least functional to replace them. I don't think every programmer should be a library programmer, but I think that libraries should not be a black box, to use it well, you should know how it works in a high level. I don't intend to make everyone a C programmer, that builds libraries for every project, but maybe, just maybe, I can inspire someone to not include one more node module in their project, and instead, write a simple class that does what they need.


## Extra 

But building this project on C and C++ was not a big challenge, after all, that is my experience. I wanted to go further and practice some new and hyped languages, that is where Rust and Zig come in.

### Zig

Zig is a new systems programming language, that is designed to be simple, fast and safe. It gives the same control as C, but with modern features and a more expressive syntax. I wanted to see how Zig would handle the task of building a key-value store, and I was not disappointed. The language is simple and expressive, and it is a joy to write in. The Zig code is also simple and straightforward, and it is not much different from the C code, but it is more expressive and safer. And Zig tooling also includes a built-in C import system, so that was a cheap way to import the header file of the Mapper lib, so I can implement it functions in Zig, compile it into a library and use it in the server, just like the C version, without any changes (and that is the power of a simple C API).

```zig
const std = @import("std");

const MAX_KEY_SIZE = 2048;
const MAX_VALUE_SIZE = 1 << 20;
const HASH_MAP_SIZE = 101;
const NUMBER_OF_MAPS = 100;

const Value = []u8;
const Key = []u8;

pub const Entry = struct {
    key: Key,
    value: Value,
    next: ?*Entry,
};

var maps: [NUMBER_OF_MAPS]?[]?*Entry = undefined;
var maps_count: usize = 0;
```

The logic is exactly the same as in C, but the expressiveness of Zig allows for even simpler definitions. The Zig Entry type does not need to include the length of the key and value, as Zig slices include the length of the slice, so that is a small improvement over the C version. The rest of the code is also simple and straightforward, and it is not much different from the C code, with exception to errors, that Zig forces you to handle in all cases. That is a good thing, but I can't help to feel that it is a little verbose, and that I did not need to handle allocation errors in a usual system. If I can't allocate memory, I can't do anything, so I would rather just panic and let the OS handle it. The out of memory case is useful for some cases, like embedded, but I think that it should be opt-in.
```zig
export fn init_map() i32 {
    if (maps_count >= NUMBER_OF_MAPS) {
        std.debug.print("[ZIG_MAPPER_ERROR]Maximum number of maps reached\n", .{});
        return -1;
    }
    const map_id: usize = @intCast(maps_count);
    maps_count += 1;
    const allocating = allocator.alloc(?*Entry, HASH_MAP_SIZE);
    if (allocating) |array| {
        for (array) |*entry| {
            entry.* = null;
        }
        maps[map_id] = array;
    } else |err| {
        std.debug.print("[ZIG_MAPPER_ERROR]Failed to allocate memory: {}\n", .{err});
        return -1;
    }

    std.debug.print("[ZIG_MAPPER_INFO]Creating new map {}\n", .{map_id});
    return @intCast(map_id);
}
```

It was fun to write the Zig version of the Mapper lib, and it was a good exercise to see how Zig handles systems programming tasks. The only problem for me is that the Zig language server is not great at all, making a lot of small mistakes only be found at compile time, and the error messages are not the best, but I think that is a matter of time to improve. The Zig code is simple and expressive, and it is a joy to write in, and I think that Zig has a bright future ahead of it.

And, as it should be, it just works with the server, I just compiled it into a shared library and linked it with the server. So now I can alternate between the C and Zig version just by compiling it, or rename to what the server expects, and it works just fine, no leaks, no bugs, no problems.

Will it replace C? I don't think so in any close future, but it is a good alternative for some cases, treating errors as values is a good thing, much better than raising exceptions, and the syntax is much more expressive than C. I think that Zig has a bright future ahead of it, and I am excited to see where it goes, but I do not expect it to replace C anytime soon.


### Rust

First of all, I do not like Rust. I do think the borrow checker is a good idea and an interesting concept to make code more memory safe. This being said, I don't want a code coach on my compiler. Damn, I even got camelCase warnings on my code. I know that I can disable it, but it bugs me that I have to opt out of some case verification. In some other cases, the compiler is just too strict, and I don't want to fight with the compiler, I want to write code, I want to make some things just work.

But even with these considerations, I must admit that Rust is an incredible language when it comes to 2 things: make servers and manipulate strings. And that is literally what my C++ server was doing, so I decided to give it a try. And I was not disappointed, the Rust server was much more straightforward to write than the C++ version. The typed enums and pattern matching make it easy to handle different types of requests, and the error handling is much simpler. Overall, a much better developer experience than C++ (but it could be even better if the compiler wasn't such a b*tch).

```rust
#[derive(Debug)]
enum RequestOrError {
    GetRequest { key: String },
    SetRequest { key: String, value: String },
    DestroyRequest { map_id: Option<i32> },
    HandshakeRequest { map_id: Option<i32> },
    Error(String),
}
fn process_task(task: RequestOrError, mapId: &mut i32) -> Result<String, String> {
    match task {
        RequestOrError::GetRequest { key } => {
            let key_c = Vec::from(key);
            let mut buffer = vec![0; 1 << 20];
            let res = unsafe {
                get_val(*mapId as i32, 
                        key_c.as_ptr() as *const u8, 
                        key_c.len() as i32, 
                        buffer.as_mut_ptr() as *mut u8)
            };
            if res != -1 {
                let buff_size = res as usize;
                Ok(String::from_utf8_lossy(&buffer[..buff_size]).to_string())
            } else {
                Err("Failed to get value".to_string())
            }
        }
        RequestOrError::SetRequest { key, value } => {
            let key_c = Vec::from(key.clone());
            let value_c = Vec::from(value.clone());
            let res = unsafe {
                set_val(
                    *mapId as i32,
                    key_c.as_ptr() as *const u8,
                    key.len() as i32,
                    value_c.as_ptr() as *const u8,
                    value.len() as i32,
                )
            };
            if res != -1 {
                Ok("OK".to_string())
            } else {
                Err("Error setting value".to_string())
            }
        }
        RequestOrError::DestroyRequest { map_id } => {
            if let Some(map_id) = map_id {
                unsafe {
                    destroy_map(map_id);
                }
            } else {
                unsafe {
                    destroy_all_maps();
                }
            }
            Ok("OK".to_string())
        }
        RequestOrError::HandshakeRequest { map_id } => {
            if let Some(map_id) = map_id {
                *mapId = map_id;
                Ok(map_id.to_string())
            } else {
                let res = unsafe { init_map() };
                if res == -1 {
                    Err("Error initializing map".to_string())
                } else {
                    *mapId = res;
                    Ok(res.to_string())
                }
            }
        }
        RequestOrError::Error(e) => Err(e),
    }
}
```

Even to call the C API was not a big hurdle. It is not as easy as to just include some header file and call the functions, but it is not hard either, you just need to declare the API in the rust syntax inside an 'extern "C"' block. The only downside is that you have to use unsafe blocks to call the C functions, but that is to be expected when calling unsafe code.

```rust
extern "C" {
    fn init_map() -> i32;
    fn get_val(map_id: i32,key: *const u8, key_len: i32, buffer: *mut u8) -> i32;
    fn set_val(map_id: i32, key: *const u8, key_len: i32, value: *const u8, value_len: i32) -> i32;
    fn destroy_map(map_id: i32);
    fn destroy_all_maps();
}
```

The performance also is not bad, completely compatible with the C++ version, handling about 10k requests per second, when sending about 100k short requests.
```
./benchmark 8080 100000
Starting endurance test with 100000 set operations
Setting: [==================================================] 100%  13288.33 req/s
Starting endurance test with 100000 get operations
Getting: [==================================================] 100%  9878.44 req/s
Endurance test completed in 20.25 seconds
Requests per second: 9878.44
```

### Drag Racing Languages for no reason

Now I had 2 versions of the mapper lib, and 2 versions of the server, all this can be mixed and matched, what is the next obvious step? Drag Race the sh!t out of the code!

These are just micro-benchmarks, and do not represent the real-world performance of any language, of course. But since all these versions are compatible and were written with the same goal in mind, I think it is a fair comparison. The benchmark was run on a Ryzen 5 5600, and the results are the average of 10 runs.


As expected, the results are absolutely comparable, almost the same performance and nothing to special to point out. This test works better to show the limitations of the design and the data structures used.

