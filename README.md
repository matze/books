# About [![Build Status](https://travis-ci.org/matze/books.png)](https://travis-ci.org/matze/books).

_Books_ is a GTK+ 3 application to view and manage books in EPUB format.


## Installation

If you compile from the GitHub sources make sure to get the autotools, intltool,
autopoint and libtool first. On Ubuntu 12.04, all you need is:

    $ sudo apt-get install intltool autopoint libtool
    $ ./autogen.sh

Additionally, you need to install the dependencies:

    $ sudo apt-get install libarchive-dev libsqlite3-dev libwebkitgtk-3.0-dev libgtk-3-dev libxml2-dev

Configure, compile and install _Books_ with

    $ ./configure
    $ make && sudo make install


## Contributions

If you feel Books need enhancements or bug fixes, don't hesitate to file a bug
on the GitHub [issue tracker][] or fork and do a pull request.

If you want to see Books in your language, you can translate the application at
[Transifex][].

[issue tracker]: https://github.com/matze/books/issues
[Transifex]: https://www.transifex.com/projects/p/books/
