Foobar2000 BruteFIR DSP plug-in
===============================

Description
-----------

This is a Foobar2000 DSP plug-in that provides a 31 band
1/3 ISO octave equalizer and DRC convolver using code ported 
from the linux program BruteFIR.  While there are many other 
equalizer plug-ins available, I have found that convolution 
provides the cleanest equalization from a sonic standpoint.  

Since the convolver filter requires a sampling rate that matches 
the input signal, using it as an equalizer typically presents 
some difficulty if streams with different sampling rates need 
to be processed.  This plug-in automatically renders filter 
coefficients for the equalizer based on the sampling rate of
the playback stream.


Use and Configuration
---------------------

The rendered equalizer impulse response is cached to disk and 
stored in WAV format.  The files can be found in tne foo_dsp_bfir 
subdirectory in the Foobar profile directory.

In addition to the equalizer, this plug-in also supports convolving 
with a set of coefficients stored in an impulse file for purposes 
of digital room correction.  The libsndfile library is used to 
read the sound file, so any format that is supported by that 
library may be used.  

The plug-in supports up to 3 impulse files with the intent that 
3 versions of the same impulse file at different sampling rates 
can be used, however, the 3 impulses may be different.  The
plug-in selects the appropriate impulse file by matching the 
sampling rate of the impulse file and the playback stream.  

If more than one impulse file can be used, they are preprocessed 
by convolving them into a single impulse file.  If the equalizer
is enabled, it is also convolved with the specified impulse 
file(s).  The resulting impulse response is cached to disk
and stored in WAV format.

The equalizer and impulse file configuration may be saved to
and loaded from disk using the DSP configuration panel.  The
configuration is stored in JSON format.

Note that the FFTW DLL's (libfftw3-3.dll and libfftw3f-3.dll) 
and the libsndfile DLL (libsndfile-1.dll) must be in the same 
directory as the plug-in DLL for proper operation.

The plug-in features a command-line interface server for
remote control of settings.  Commands consist of an
operation code and data string separated by a space
and terminated by a carriage return.  If no data string 
is present, the current setting is returned.  If a data 
string is present, the current setting is updated and 
"OK" or "ERR" is returned depending on if the update was 
successful.  The supported commands are:

    EQMx <-10..10>    get/set EQ magnitude where x is the band number (0..30)  
    EQEN <0 | 1>      get/set EQ enable  
    F1EN <0 | 1>      get/set file 1 enable  
    F2EN <0 | 1>      get/set file 2 enable  
    F3EN <0 | 1>      get/set file 3 enable  
    EQLV <-100..100>  get/set EQ level  
    F1LV <-200..200>  get/set file 1 level  
    F2LV <-200..200>  get/set file 2 level  
    F3LV <-200..200>  get/set file 3 level  
    F1FN <file path>  get/set file 1 filename  
    F2FN <file path>  get/set file 2 filename  
    F3FN <file path>  get/set file 3 filename  
    F1MD              get file 1 metadata
    F2MD              get file 2 metadata
    F3MD              get file 3 metadata
    DIR <dir path>    list directory
    CLOSE             close client connection  

Setting the filename to "?" (without quotes) indicates no file
and resets metadata and file level.

The directory listing returns a concatenated string with items
delimited by "|" (without quotes).  The first item is the full
path to the directory.  Items are file/directory names only
without the path.  Directories are prefixed with ":" (without 
quotes) and sorted first.  If the directory path argument
is omitted, the default directory (the application path) is 
used.



Compilation
-----------

You will need:

Foobar2000 SDK (http://www.foobar2000.org/SDK)  
Include files from the Windows Template Library (http://sourceforge.net/projects/wtl/)  
Boost C++ libraries (www.boost.org) (You can find pre-built binaries at boostpro.com)  

The JSON Spirit, FFTW and libsndfile libraries are present in the solution
directory or may be obtained from:

http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx
http://www.fftw.org/
http://www.mega-nerd.com/libsndfile/

Project and solution files are currently for Visual Studio 2010.
