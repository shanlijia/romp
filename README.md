Refactoring and improving ROMP is in progress.

### ROMP 
A dynamic data race detector for OpenMP program 

### System Requirements
Operating Systems: Linux

Architecture: x86_64

Compiler: gcc 9.2.0 (recommended); c++17 support is required;

### Installation Guide

By default, we use `spack` to manage the installation of ROMP and related packages.
This should be the default way of installing ROMP. Installation using CMake 'manually' 
is described in a separate section.

#### Install ROMP using Spack

1. install `spack`
* Checkout my forked branch of `spack`. It contains changes to package.py for `llvm-openmp`, `dyninst`, and 
the pacakge spec for `romp`:

```
   git clone git@github.com:zygyz/spack.git
   git checkout romp-build
```
* For the installation of Spack, please refer to the guide in Spack project readme. 

2. install environment modules 
* We use `modules` to manage environment paths for installed pacakges. To check if environment modules is installed
  on your system, please try `module` command in terminal. 
  To install environment modules, please refer to section 'Bootstrapping Environment Modules' in      http://hpctoolkit.org/software-instructions.html#Building-a-New-Compiler
  
3. install gcc 9.2.0
* We use a designated compiler to build all packages. Before building new compiler, please follow steps described in
  'Building a New Compiler' shown in the link above.
* fetch and install gcc 9.2.0 using spack 
 ``` spack install gcc@9.2.0```
* using the gcc 9.2.0
  * Again, please follow steps in section 'Using the New Compiler' shown in the link above.
    
3. install ROMP
  ```
  spack install romp@master
  ```
##### Setup environment variables 
 Setup environment variables so that we can run ROMP.
 * First get the correct version of dyninst that ROMP uses. Because some other pacakges may also depend 
 on dyninst and the variant of dyninst may be different. This difference is marked by different hash 
 ```
  spack spec -l romp
 ```
 find the hash of dyninst that romp depends on. For example, if the hash is `jmaisrn`, then
 ```
 export DYNINSTAPI_RT_LIB=`spack location --install-dir dyninst/jmaisrn`/lib/libdyninstAPI_RT.so
 ```
 One can put the export statement above in .bashrc as long as the installed dyninst packages stays intact.
 * Then load relevant modules, for example: 
 ```
   module load gcc-9.2.0-gcc-7.5.0-fp5any7
   module load llvm-openmp-romp-mod-gcc-9.2.0-i46hs56
   module load glog-0.3.5-gcc-9.2.0-sxceiv3
   module load dyninst-10.1.2-gcc-9.2.0-jmaisrn
   module load romp-master-gcc-9.2.0-dcspvk5
 ```
 To get the exact modules on your system, please use `module avail` command. 
* Then export ROMP_PATH which is used by instrumentation client
 ```
   export ROMP_PATH=`spack location --install-dir omp`/lib/libromp.so
 ```
#### Install ROMP using CMake
People may want a faster development and iteration experience when debugging and developing ROMP. Installation using 
spack requires changes to be committed to remote repos. ROMP's cmake files make it possible to build ROMP and a local copy of dyninst without using spack. Note that we still use spack to install some dependent libraries.

1. install `spack`
*  same as described in above section
2. install dependent packages
* glog
  ```
  spack install glog
  ```
* llvm-openmp
  ```
  spack install llvm-openmp@romp-mod
  ```
* dyninst
  ```
  spack install dyninst@10.1.2~openmp
  ``` 
3. setup envorinment variables for building
* Note that the following environment variable setting does not need to go into .bashrc, it is only
  used at build time. For example: 
  ```
   module load glog-0.3.5-gcc-9.2.0-sxceiv3
   module load boost-1.72.0-gcc-9.2.0-j2rmx26
   module load dyninst-10.1.2-gcc-9.2.0-jmaisrn
   module load intel-tbb-2020.2-gcc-9.2.0-ybbbcc7
   module load llvm-openmp-romp-mod-gcc-9.2.0-i46hs56
  ```
4. build and install romp
* suppose romp is located in `/home/to/romp`
  ```
   cd /home/to/romp
   mkdir build
   mkdir install
   cd build
         
   cmake -DCMAKE_CXX_FLAGS=-std=c++17 
         -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc 
         -DCMAKE_INSTALL_PREFIX=`pwd`/../install ..
   make
   make install
  ```
##### Setup environment variables 
Setup environment variables so that we can run ROMP. 
* The setting is almost the same as the setting used when romp is installed by spack.
  Suppose romp root path is `/home/to/romp`:
```
 module load gcc-9.2.0-gcc-7.5.0-fp5any7
 module load llvm-openmp-romp-mod-gcc-9.2.0-i46hs56
 module load glog-0.3.5-gcc-9.2.0-sxceiv3
 module load dyninst-10.1.2-gcc-9.2.0-jmaisrn
 export DYNINSTAPI_RT_LIB=`spack location --install-dir dyninst/jmaisrn`/lib/libdyninstAPI_RT.so
 export ROMP_PATH=/home/to/romp/install/lib/libromp.so
```

### Compile and instrument a program
* suppose an OpenMP program is `test.cpp`
1. compile the program so that it links against our llvm-openmp library
```
g++ -g -fopenmp -lomp test.cpp -o test
```
* one can `ldd test` to check if `libomp` is our spack installed one, which contains changes to support OMPT callbacks
* if the linkage is incorrect, e.g., it uses system library, check if the library name mismatches:
```
cd `spack location --install-dir llvm-openmp`/lib
```
* it is possible that linker wants to find `libomp.so.5` but the spack installed lib only contains `libomp.so`. In this case, create a symlink `libomp.so.5->libomp.so` yourself

2. instrument the binary
* use dyninst instrument client `InstrumentMain` to instrument the binary
```
InstrumentMain --program=./test
```
* this would generate an instrumented binary: `test.inst`
3. check data races for a program
* (optional) turn on line info report.
```
export ROMP_REPORT_LINE=on
```
when enabled, this would print all data races found with line information
* (optional) turn on on-the-fly data race report
```
export ROMP_REPORT=on
```
when enabled, once a data race is found during the program execution, it is reported. Otherwise,
all report would be generated after the execution of the program
* run `test.inst` to check data races for program `test`

### Running DataRaceBench
* check out my forked branch `romp-test` of data race bench, which contains modifications to scripts to support running romp
 https://github.com/zygyz/dataracebench 
```
git clone git@github.com:zygyz/dataracebench.git
git checkout romp-test
cd dataracebench
./check-data-races.sh --romp
```
