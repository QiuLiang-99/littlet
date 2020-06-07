﻿/*
    Copyright (c) 2007-2013 Contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testutil.hpp"

int main (void)
{
    setup_test_environment();
    void *ctx = zmq_ctx_new ();
    assert (ctx);

    void *frontend = zmq_socket (ctx, ZMQ_DEALER);
    assert (frontend);
    int rc = zmq_bind (frontend, "inproc://timeout_test");
    assert (rc == 0);

    //  Receive on disconnected socket returns immediately
    char buffer [32];
    rc = zmq_recv (frontend, buffer, 32, ZMQ_DONTWAIT);
    assert (rc == -1);
    assert (zmq_errno() == EAGAIN);
    
    //  Check whether receive timeout is honored
    int timeout = 250;
    rc = zmq_setsockopt (frontend, ZMQ_RCVTIMEO, &timeout, sizeof (int));
    assert (rc == 0);

    void* stopwatch = zmq_stopwatch_start();
    rc = zmq_recv (frontend, buffer, 32, 0);
    assert (rc == -1);
    assert (zmq_errno () == EAGAIN);
    unsigned int elapsed = zmq_stopwatch_stop(stopwatch) / 1000;
    assert (elapsed > 200 && elapsed < 300);

    //  Check that normal message flow works as expected
    void *backend = zmq_socket (ctx, ZMQ_DEALER);
    assert (backend);
    rc = zmq_connect (backend, "inproc://timeout_test");
    assert (rc == 0);
    rc = zmq_setsockopt (backend, ZMQ_SNDTIMEO, &timeout, sizeof (int));
    assert (rc == 0);

    rc = zmq_send (backend, "Hello", 5, 0);
    assert (rc == 5);
    rc = zmq_recv (frontend, buffer, 32, 0);
    assert (rc == 5);

    //  Clean-up
    rc = zmq_close (backend);
    assert (rc == 0);
    
    rc = zmq_close (frontend);
    assert (rc == 0);
    
    rc = zmq_ctx_term (ctx);
    assert (rc == 0);

    return 0 ;
}
