
#define DEBUG 0
#define debug(...) do { if(DEBUG) printf(__VA_ARGS__); } while (0)
#define debug_hexdump(...) do { if(DEBUG) printf_hexdump(__VA_ARGS__); } while (0)

// Bluetooth Device address to connect
const char remote_addr_string[] = "AF-17-70-81-63-25";
