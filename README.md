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
./libt <path to function file>
```
To request a function run: write your own client ;(

Sample client is given in ```clients/faas_client_sample.c``` and is also built into ```build/clients``` dir.
Also, sample libraries are provided in ```samples/``` dir.
