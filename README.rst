
==================================
nginx stub_requests module
==================================

The `ngx_http_stub_requests` module provides provides access to basic requests information.


Dependencies
============
* Sources for nginx 1.x.x, and its dependencies.

* `ngx_http_stub_requests` will load jQuery from "//ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js"

Building
========

1. Download nginx::

   $ wget http://nginx.org/download/nginx-1.9.2.tar.gz

2. Unpack the nginx sources::

    $ tar xzf nginx-1.9.2.tar.gz

3. Download the module::

   $ wget https://github.com/atomx/nginx-http-stub-requests/archive/v1.0.1.tar.gz

4. Unpack the sources for the module::

    $ tar xzf nginx-http-stub-requests-1.0.1.tar.gz

5. Change to the directory which contains the nginx sources, run the
   configuration script with the desired options and be sure to put an
   ``--add-module`` flag pointing to the directory which contains the source
   of the module::

    $ cd nginx-1.x.x
    $ ./configure --add-module=../nginx-http-stub-requests-1.0.1  [other configure options]

6. Build and install the software::

    $ make && sudo make install

7. Configure nginx using the module's configuration directives_.


Example
=======

You can enable stub_requests by adding the following lines into
a ``location`` section in your nginx configuration file::

  stub_requests;


You can find an example here: http://dubbelboer.com/requests

.. image:: https://github.com/atomx/nginx-http-stub-requests/blob/master/example.png


Directives
==========

stub_requests
~~~~
:Syntax:  ``stub_requests``
:Default: --
:Context: location
:Description:
  The basic request information will be accessible from the surrounding location.

  The location can not use any parameters as `ngx_http_stub_requests` will use these for it's enable
  and disable functionality.

stub_requests_shm_size
~~~~~~~~~~~~~~~~~~~~
:Syntax: ``stub_requests_shm_size`` *size-in-bytes*
:Default: ``4096k``
:Context: http
:Description:
  The module keeps information about requests in shared memory so each worker can
  generate the requests page. The information includes the requested uri and the
  user agent.

  When the shared memory is full an error will be logged. This will not stop the
  request from continuing normally.
 

Testing
==========

Make sure to compile nginx with the ngx_echo module: http://wiki.nginx.org/HttpEchoModule::

    $ ./configure --add-module=../nginx-http-stub-requests-1.0.1 --add-module=../echo-nginx-module  [other configure options]

After that you can run the test against the nginx binary::

    $ cd test
    $ NGINX=/home/erik/nginx-1.9.1/objs/nginx ./test.sh

