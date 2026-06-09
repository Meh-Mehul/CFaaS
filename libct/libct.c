#include "libct.h"
#include <stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>


int lc_get_randid(){
  srand(time(NULL));
  return (rand()%(MAX_RAND-MIN_RAND+1))+MIN_RAND;
}


// (to be checked) A function to recieve and write
// to a temporary file in the format "templib/lib_<rand_id>.c"
// NOTE: The client must send a long integer for file size first, 
// and then only send the file.
char* recv_file_n_save(int socket){
    int random_uid = lc_get_randid();
    char* dest_fname = malloc(sizeof(char)*FILENAME_MX_SZ);
    snprintf(dest_fname, sizeof(dest_fname), "templib/lib_%d.c", random_uid);
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
    return dest_fname;
}

// This function checks whether the recieved
// temporary file is conformant with the requirements
//  for being a runnable library or not?
// Rules in: /cfaas/libct/readme.txt
bool validate_file(char* filepath){
  
}


int compile_to_lib(char* filepath){
  
}
