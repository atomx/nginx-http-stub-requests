
==================================
nginx stub_requests module
==================================

The `ngx_http_stub_requests` module provides provides access to basic requests information.


Dependencies
============
* Sources for nginx 1.x.x, and its dependencies.


Building
========

1. Unpack the nginx\_ sources::

    $ tar zxvf nginx-1.x.x.tar.gz

2. Unzip the sources for the digest module::

    $ unzip master.zip

3. Change to the directory which contains the nginx\_ sources, run the
   configuration script with the desired options and be sure to put an
   ``--add-module`` flag pointing to the directory which contains the source
   of the digest module::

    $ cd nginx-1.x.x
    $ ./configure --add-module=../nginx-http-stub-requests-master  [other configure options]

4. Build and install the software::

    $ make && sudo make install

5. Configure nginx using the module's configuration directives_.


Example
=======

You can enable stub_requests by adding the following lines into
a ``location`` section in your nginx configuration file::

  stub_requests;


Directives
==========

stub_requests
~~~~
:Syntax:  ``stub_requests``
:Default: --
:Context: location
:Description:
  The basic request information will be accessible from the surrounding location.
  

Testing
==========

Make sure to compile nginx with the ngx_echo module: http://wiki.nginx.org/HttpEchoModule::

    $ ./configure --add-module=../nginx-http-stub-requests-master --add-module=../echo-nginx-module  [other configure options]

After that you can run the test against the nginx binary::

    $ cd test
    $ NGINX=/home/erik/nginx-1.9.1/objs/nginx ./test.sh

