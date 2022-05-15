# set environments
$ENV{PATH} = 
   'c:\buildenv\deps\tools\usr\bin;'.
   'c:\buildenv\deps\python27;'.
   'c:\buildenv\deps\tcl85\bin;'.
   'c:\buildenv\deps\lib\gettext;'.
   'c:\buildenv\deps\lib\openssl\bin;'.
   'c:\buildenv\deps\lib\libxml2\bin;'.
   'c:\buildenv\deps\lib\libxslt\lib;'.
   'c:\buildenv\deps\lib\libiconv;'.
   'c:\buildenv\deps\lib\zlib;'.$ENV{PATH};
#$ENV{CONFIG} = "Debug";
$ENV{DOCROOT} = 'c:\buildenv\deps\docbook';
$ENV{PERL5LIB} = $ENV{PERL5LIB}.';c:\buildenv\deps\IPC-Run-0.94';
