#include "libct.h"
#include <stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<assert.h>
#include<dlfcn.h>
#include"../common.h"

int lc_get_randid(){
  srand(time(NULL));
  return (rand()%(MAX_RAND-MIN_RAND+1))+MIN_RAND;
}

// delegate library for deletion (not-strict gurentee)
void del_lib(char* filepath){
  remove(filepath);
}

// (to be checked) A function to recieve and write
// to a temporary file in the format "templib/lib_<rand_id>.c"
// NOTE: The client must send a long integer for file size first, 
// and then only send the file.
char* recv_file_n_save(int socket){
    int random_uid = lc_get_randid();
    DEBUG("Generated random UID for file:");
    char* dest_fname = malloc(sizeof(char)*FILENAME_MX_SZ);
    snprintf(dest_fname, FILENAME_MX_SZ, "./templib/%d.c", random_uid);
    char buffer[BUFFER_SIZE];
    long file_size = 0;
    ssize_t bytes_recv = 0;
    // first the client sends the filesize
    // (one long integer)
    if(recv(socket, &file_size, sizeof(file_size), 0)<=0){
      perror("ERECV_LIB: Failed to recv library.");
      return NULL;
    }

    FILE *file = fopen(dest_fname, "wb");
    if(file == NULL){
      perror("EOPEN_TMPFILE: Could not write the file");
      return NULL;
    }         
    long total_bytes_written = 0;
    while(total_bytes_written < file_size){
      long rem_bytes = file_size - total_bytes_written;
      size_t chunk_sz = (rem_bytes < BUFFER_SIZE)?rem_bytes:BUFFER_SIZE;
      bytes_recv = recv(socket, buffer, chunk_sz, 0);
      if(bytes_recv <= 0){
        perror("ERECV_FILE: Error while recving file.");
        fclose(file);
        return NULL;
      }
      size_t bytes_written = fwrite(buffer,sizeof(char),bytes_recv,file);
      if(bytes_written < (size_t)bytes_recv){
        perror("EWRITE_FILE: Error while writing to file");
        fclose(file);
        return NULL;
      }
      total_bytes_written += bytes_written;
    }
    fclose(file);
    return dest_fname;
}

// IMPORTANT: This is not for temporary file,
// instead its a check after compilation 
// This function checks whether the recieved
// compiled final file is conformant with the requirements
//  for being a runnable library or not?
// if not, its auto-deleted
// Rules in: /cfaas/libct/readme.txt
// unfortunatelt C does not allow for type check ;(
bool validate_file(char* filepath){
  int id = get_id_from_fp(filepath);
  void* handle = dlopen(filepath, RTLD_LAZY);
  if(!handle){
    perror("E_LOADING_LIB");
    return false;
  }    
  dlerror();
  void* symbol = dlsym(handle, "fn");
  // TODO: Implement an AST based checker for type
  // during the pre-compile phase of the library
  // (yea, im not gonna do this)
  const char* error = dlerror();
  if(error != NULL){
    perror("E_NOSYMBOL: Symbol not present");
    del_lib(filepath);// call for deletion
    return false;
  }
  return true;
}

// this function literally compiles
// the temp file to a shared lib
// and returns the path
// on failure, it returns NULL
char* compile_to_lib(char* filepath){
  char command[1024];
  char* dest_path = malloc(512*sizeof(char));
  int id = get_id_from_fp(filepath);
  snprintf(dest_path, 512, "./libs/%d.so", id);
  snprintf(command, sizeof(command), "gcc -fPIC -shared -o %s %s", dest_path, filepath);
  int status = system(command);
  if(status != 0){
    perror("ECOMPILE_LIB: Could not compile library");
    return NULL;
  }
  remove(filepath);
  return dest_path; 
}
