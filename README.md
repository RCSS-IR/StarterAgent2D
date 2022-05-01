# Starter Agent2D  
Starter Agent2D Source Code  
## Installation LibRCSC
Download last release of StarterLibRCSC from this [Link](https://github.com/RCSS-IR/StarterLibRCSC)
```bash
cd StarterLibRCSCPath
./configure --prefix=/usr/local/StarterLibRCSC/
make  
sudo make install
```  

## How To Use StarterAgent2D
### First Time
```bash
cd StarterAgent2DPath
./configure
make  
```
### After Any Change
```bash
cd StarterAgent2DPath/src
make
```
### Run
```bash
cd src
./start.sh -t teamname
```
### Fix Problems

#### Bug 1
```bash
CDPATH="${ZSH_VERSION+.}:" && cd . && aclocal-1.15 -I m4
/bin/bash: aclocal-1.15: command not found
make: *** [Makefile:348: aclocal.m4] Error 127
```
#### Solution 1
```bash
cd StarterAgent2DPath
./bootstrap
./configure
make  
```

* If it's useful for you, please touch star.
