#include "../libres/libres.h"

#define MAX_FILE_SIZE_BYTES 4096

// plain old socket code, i definitely will write myself
char* recv_file_n_save(int socket);

// gotta check whether the file has a symbol of "fn" of type int (*)(char*)
bool validate_file(char* filepath);

// compile and save to lib and return the random id
int compile_to_lib(char* filepath);
