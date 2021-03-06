
[ Building ]

We offer a 'make world' script that retrieves nearly everything from
the Internet, creates harmonious configurations, and then builds everything.
The build process generates many side products, and thus you should 
build within a dedicated directory (and not within the Afterburner directory).

To build, change into your dedicated build directory, and execute:
make -f path/to/afterburner/Makefile
make menuconfig
make world

You can skip the 'make menuconfig' and accept the defaults.  If you want to 
specify the build components and their versions, then you must 
run 'make menuconfig' and tailor the configuration to your preferences.
The default configuration will build components that have been under test
for over a year, and thus are fairly old.

Other commands:
  make downloads
    - Lists the files that we retrieve from the Internet.
  make download_now
    - Retrieve the files from the Internet immediately.
  make cvs
    - Lists the CVS repositories that we retrieve from the Internet.
  make cvs_now
    - Retrieve the CVS repositories from the Internet now.


[ Running ]

The build process installs all of the boot files into your configured
boot directory (by default, the "boot" subdirectory within the build 
directory).  The boot directory includes an example GRUB menu.

You can test by booting within QEMU:
make run-qemu


[ Problems ]

If you have problems building Antlr, upgrade your compiler.  If you have 
problems building other components, then you probably need to downgrade 
your compiler.  You can build a tested compiler (gcc 3.4.4) within the 
Afterburn build environment by enabling it within 'make menuconfig'.

