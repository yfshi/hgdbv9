# set environments
$ENV{PATH} = 
   'c:\buildenv\deps\tools\usr\bin;'.
   'c:\buildenv\deps\python27_x64;'.
   'c:\buildenv\deps\tcl85_x64\bin;'.
   'c:\buildenv\deps\lib_x64\gettext;'.
   'c:\buildenv\deps\lib_x64\openssl\bin;'.
   'c:\buildenv\deps\lib_x64\libxml2\bin;'.
   'c:\buildenv\deps\lib_x64\libxslt\lib;'.
   'c:\buildenv\deps\lib_x64\libiconv;'.
   'c:\buildenv\deps\lib_x64\zlib;'.$ENV{PATH};
#$ENV{CONFIG} = "Debug";
$ENV{DOCROOT} = 'c:\buildenv\deps\docbook';
$ENV{PERL5LIB} = $ENV{PERL5LIB}.';c:\buildenv\deps\IPC-Run-0.94';
