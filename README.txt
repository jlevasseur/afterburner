
[ Building ]

make world

If you have problems building Antlr, upgrade your compiler, or build
a compiler within the Afterburn build environment: make build-gcc

If you have problems with Python on your system, you can build a standard
Python within the Afterburn build environment: make build-python


[ Additional commands ]

Many of the build targets have the following rules defined:
    make rebuild-*
    make clean-*
    make uninstall-*


[ Mercurial integration ]

We distribute the Afterburner projects via the CVS system, because CVS performs well as a central repository.  It is outclassed by Mercurial (Hg) for normal
revision control activities, and we recommend that you use Mercurial for
maintaining your local source tree.  Both Mercurial and CVS comfortably
live together in the same working directory.

