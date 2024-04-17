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
