#include "mapper.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_SIZE 2048
#define MAX_VALUE_SIZE 10 << 20
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

int hash(key key, int key_len) {
  int hash = 0;
  for (int i = 0; i < key_len; i++) {
    hash += key[i];
  }
  hash = hash < 0 ? -hash : hash;
  return hash % HASH_MAP_SIZE;
}

int init_map() {
  if (maps_count >= NUMBER_OF_MAPS) {
    LOG_ERROR("Maximum number of maps reached");
    return -1;
  }
  maps[maps_count] = (hash_map_t)malloc(sizeof(entry *) * HASH_MAP_SIZE);
  for (int i = 0; i < HASH_MAP_SIZE; i++) {
    maps[maps_count][i] = NULL;
  }
  LOG_INFO("Creating new map %d", maps_count);
  return maps_count++;
}

int set_val(int map_id, const char *key, int key_len, const char *value,
            int value_len) {
  if (map_id < 0 || map_id >= maps_count || key_len > MAX_KEY_SIZE ||
      value_len > MAX_VALUE_SIZE) {
    LOG_ERROR("Invalid map_id or key/value size");
    return -1;
  }
  int hash_key = hash(key, key_len);
  entry *new_entry = (entry *)malloc(sizeof(entry));
  char *key_copy = (char *)malloc(key_len);
  char *value_copy = (char *)malloc(value_len);
  memcpy(key_copy, key, key_len);
  memcpy(value_copy, value, value_len);
  new_entry->key_len = key_len;
  new_entry->value_len = value_len;
  new_entry->key = key_copy;
  new_entry->value = value_copy;
  new_entry->next = NULL;

  LOG_DEBUG("Creating new entry on map %d", map_id);
  LOG_DEBUG("\thash_key: %d", hash_key);
  LOG_DEBUG("\tkey: -%*s-", key_len, key);
  LOG_DEBUG("\tvalue: -%*s-\n", value_len, value);
  entry *current = maps[map_id][hash_key];
  if (current == NULL) {
    maps[map_id][hash_key] = new_entry;
    return 0;
  } else if (key_len == current->key_len &&
             memcmp(current->key, key, key_len) == 0) {
    memcpy(current->value, value, value_len);
    current->value_len = value_len;
    free(key_copy);
    free(value_copy);
    free(new_entry);
    return 0;
  }
  while (current->next != NULL) {
    current = current->next;
  }
  current->next = new_entry;
  return hash_key;
}

int get_val(int map_id, const char *key, int key_len, char *value) {
  if (map_id < 0 || map_id >= maps_count) {
    LOG_ERROR("map_id out of range\n");
    LOG_ERROR("\tmap_id: %d\n", map_id);
    LOG_ERROR("\tmaps_count: %d\n", maps_count);
    return -1;
  }
  int hash_key = hash(key, key_len);

  LOG_DEBUG("Getting value on map %d\n", map_id);
  LOG_DEBUG("\thash_key: %d\n", hash_key);
  LOG_DEBUG("\tkey: -%*s-\n", key_len, key);
  entry *current = maps[map_id][hash_key];
  while (current != NULL) {
    if (key_len == current->key_len &&
        memcmp(current->key, key, key_len) == 0) {
      memcpy(value, current->value, current->value_len);
      return current->value_len;
    }
    current = current->next;
  }
  return -1;
}

void destroy_map(int map_id) {
  if (map_id < 0 || map_id >= maps_count) {
    LOG_ERROR("map_id out of range: %d\n", map_id);
    return;
  }
  LOG_INFO("Destroying map %d\n", map_id);
  for (int i = 0; i < HASH_MAP_SIZE; i++) {
    entry *current = maps[map_id][i];
    while (current != NULL) {
      entry *next = current->next;
      free((void *)current->key);
      free((void *)current->value);
      free(current);
      current = next;
    }
  }
  free(maps[map_id]);
  maps[map_id] = NULL;
}

void destroy_all_maps() {
  for (int i = 0; i < maps_count; i++) {
    if (maps[i] == NULL) {
      continue;
    }
    destroy_map(i);
  }
  maps_count = 0;
}

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
