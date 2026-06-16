## CFaaS : Because why not?

So basic idea behind this is that there is one program that handles all the functionality of a Function-as-a-service (serverless) program. Just like the JSS one, this also has a manager thread managing the worker thread for their scheduling 
and state manager. This is almost what an OS does but with lesser permission management.

To read about how to send FDs over FDs : [SCM_RIGHTS](https://blog.cloudflare.com/know-your-scm_rights/).

Basically, i read the above blog and got inspired to write a load balancer, but i also wanted to do serverless once, so i just kinda merged them in one.

### Steps:
This program has no dependency on 3p libraries, so just do the following and see ```build``` and ```build/clients```:
```
make
make client
```
To load a function into the FaaS (```libct``` is inside clients dir):
```
./libct <path to function file>
```
To request a function run: write your own client ;(

Sample client is given in ```clients/faas_client_sample.c``` and is also built into ```build/clients``` dir.
Also, sample libraries are provided in ```samples/``` dir.

### CFaaS : As a TCP-level Load Balancer
This statement is kind of a stretch, but since, it distributes file descriptors over a set
of running processes, it can be seen as a TCP-level load balancer.

### CFaaS : As a severless
This much may be stated as obvious after use.


### Some notes:
#### Functions, Libraries and the clients
While writing libraries and clients, do make sure that the first argument is the input string to the
function and the second argument is the file descriptor to send messages back to, 
hence any errors propagated by the CFaaS native are also string messages only, do make
sure to write client in a way such that they are handled.

#### Function structure
Make sure to deal with any errors and edge cases a function may encounter
during its runtime inside the function itself as CFaaS currently does not handle
such cases.
The library function MUST be name ```fn``` and must have the type : ```int fn(int*, char*)```
No need of the ```main``` function inside the library code

#### Yes, i did not test it thoroughly
That would require me having any sanity left for debugging C code.
