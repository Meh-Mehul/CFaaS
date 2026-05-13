// ires's major work is to get input to char* 
// to make it passable to the <lib_func>

#define MAX_INPUT_SIZE 2048
// for now, i have assumed ID and inputsz are integers
#define TAGS_SZ_B 4       

// general function to read x bytes from an open fd 
// (although not needed) just exposing it in case needed
void* read_x_bytes(int* fd,unsigned int x);


// to resolve request string from input fd
char* resolve_input_str(int* fd, unsigned int x);

// to resolve <lid-id> from input request
int* resolve_id(int* fd);

// to resolve <req_size> from input request
int* resolve_sz(int* fd);
