Library Creator for CFaaS:

Main goal of this sub-module would be to be responsible
for creation of the libraries (the functions) for being
ran by the Service.


Rules for function being conformant:
As discussed in the libres : the function must be of type : int (*fn)(int*, char*)
and it must be named -> "fn"

So these check must be done ahead of time.

The main program for this would work as a socket server.
(sadly, ill have to write a client as well)
This server would recieve an entire c file over the socket, 
save and compile it to a library after checking its symbols
for correct structure.

As usual, im not gonna scale this, so this is gonna be a plain
get-save-compile-validate loop (yes, im gonna store them all locally.)
