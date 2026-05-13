Library Resolver for CFaaS
Im planning to keep its implementation relatively simple
-> get the ID
-> find the library in /libs
-> extract symbols
-> return fp 


Note: The core executor function will be called fn and is of the type: int (*fn) (char*)
