#include "../libres/libres.h"
#define MAX_FILE_SIZE_BYTES 409600
#define BUFFER_SIZE 4096
#define FILENAME_MX_SZ 4096
#define MAX_RAND 2000
#define MIN_RAND 1
// plain old socket code, i definitely will write myself
char* recv_file_n_save(int socket);

// gotta check whether the file has a symbol of "fn" of type int (*)(char*)
bool validate_file(char* filepath);

// compile and save to lib and return the library location
char* compile_to_lib(char* filepath);
