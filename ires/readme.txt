Input Resolver for CFaaS
Planned Structure so far:
The engine recieves some format of string from the user and the input resolver's task is to convert it to void* datatype directly
So in a way, core idea is to have an encoding and decoding scheme between char* and void* (which i dont think would be hard considering
they are the same sizes.)


Ok, so from what ive heard, it may not be easy to do in C(other way
is to LITERALLY make my own GRPC), to directly
get <generic_interface*> from char* directly, hence im resolving to 
the following soln now:
Let the execute-job to have the char* input and we resolve that 
from the socket request (the fd) and keep it to char* and pass it onto 
the lib-resolver

In this way, all the responsibilty of resolving to struct values
falls onto the <user_lib_fn>


Thus, i need to implement my own Appln layer protocol-ish kinda 
thingy, heres what i plan to do:

|---ID---||--sz--||------------req_str----------------....--|
<--4B----><--4B--><------------SZ Bytes--------------------->


Also, later on i realise that this whole job should not be done by the user-facing system, instead it must  
be done by the worker this job is scheduled onto, hence, the function remain same, except the core job 
of input resolving (as well as library) is to be done by the worker (see funcex.c).
