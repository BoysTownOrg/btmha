BTMHA - Boys Town "Master Hearing Aid" Simulation

IntroductionBTMHA provides a software platform for exploration of hearing-aid signal processing.The BTMHA program performs three primary tasks:

    (1) Establishes input and output streams.
    (2) Sets up a signal processor between the input and output.
    (3) Starts the stream and maintains its operation until finished.The inputs and outputs may be files or may be an audio device. The signal processing may be a single plugin that is precompiled in a separate file or may be a combination of multiple plugins connected by mixers into chains and banks.
The software design is intended to support three levels of users:

    (1) Research Audiologists who want the ability to easily adjust signal-processing parameters.
    (2) Application engineers who want to reconfigure existing signal-processing plugins without recompiling code.
    (3) Signal-processing engineers who want to code new algorithms.A modular design has been adopted to replacement of code with alternative implementations. The program is written entirely in C to reduce its system dependence. The installation “footprint” is kept small by limiting its software package dependence to three open-source libraries developed at BTNRH.

Source Code InstallationThe dependence of BTMHA on other software packages is limited to three BTNRH libraries:    (1) ARSC – a library of functions for platform-independent access of audio devices.    (2) SIGPRO – a library of basic signal-processing functions.    (3) CHAPRO – a library of functions for simulating hearing-aid signal processing.
Although the source code compiles and runs under three different operating systems (MacOS, Windows, and Linux), version 0.05 has only been fully tested under MacOS in a Terminal window. The following installation instructions are for the Mac.Compilation in the MacOS Terminal window requires prior installation of Xcode command-line tools. 
    % xcode-select --installIn your preferred source-code folder, clone the three requisite library repositories.

    % git clone https://github.com/BoysTownorg/arsc    % git clone https://github.com/BoysTownorg/sigpro    % git clone https://github.com/BoysTownorg/chaproIn each of the three subfolders (arsc, sigpro, chapro) build and install the library.    % cd <library name>    % cp makefile.mac Makefile    % make    % sudo make installThe final step is to install the BTMHA source code. 

    % git clone https://github.com/BoysTownorg/btmha
    % cd btmha    % cp makefile.mac Makefile    % make    % sudo make installIn addition to copying the btmha executable file to /usr/local/bin, the installationprocedure also copies example plugins to /usr/local/lib.

Command-Line Execution

The BTMHA program has a command-line interface and accepts use input from tree sources: command-line arguments, interactive mode, and list files. The command-line option -h displays a list of options.

    % btmha -h
    usage: btmha [-options] [list_file] ...
    options
    -h    display usage info
    -i    interactive mode
    -q    quiet mode
    -v    display version

A good way to discover what BTMHA can do is to use its interactive mode.

    % btmha -i
    ==== Boys Town Master Hearing Aid ====
    >>

See the User Manual (https://github.com/BoysTownorg/btmha/blob/main/btmha.pdf) for a brief tutorial.

Windows Demo InstallationTo install a precompiled version of BTMHA, download the demonstration files, which consists of the BTMHA executable for Windows and several examples of LST file, from http://audres.org/downloads/btmha-dem.zip. Right-click to "Extract All..." 

In the newly extracted btmha-dem folder, double-click btmha.exe, and, if necessary give Windows permission to run the program. 

See the User Manual (https://github.com/BoysTownorg/btmha/blob/main/btmha.pdf) for a brief tutorial.
Contributors

Contact Stephen.Neely@boystown.org or Daniel.Rasetshwane@boystown.org to contribute code to the repo.

License

Creative Commons?

