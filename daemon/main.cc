// Copyright (c) 2012, Robert Escriva
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Replicant nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Popt
#include <popt.h>

// Google Log
#include <glog/logging.h>

// po6
#include <po6/net/ipaddr.h>
#include <po6/net/hostname.h>

// e
#include <e/guard.h>

// BusyBee
#include <busybee_utils.h>

// Replicant
#include "daemon/daemon.h"

static bool _daemonize = true;
static const char* _data = ".";
static const char* _listen_host = "auto";
static unsigned long _listen_port = 1982;
static po6::net::ipaddr _listen_ip;
static bool _listen = false;
static const char* _connect_host = "127.0.0.1";
static unsigned long _connect_port = 1982;
static bool _connect = false;

extern "C"
{

static struct poptOption popts[] = {
    POPT_AUTOHELP
    {"daemon", 'd', POPT_ARG_NONE, NULL, 'd',
     "run replicant in the background", 0},
    {"foreground", 'f', POPT_ARG_NONE, NULL, 'f',
     "run replicant in the foreground", 0},
    {"data", 'D', POPT_ARG_STRING, &_data, 'D',
     "store persistent state in this directory (default: .)",
     "dir"},
    {"listen", 'l', POPT_ARG_STRING, &_listen_host, 'l',
     "listen on a specific IP address (default: auto)",
     "IP"},
    {"listen-port", 'p', POPT_ARG_LONG, &_listen_port, 'L',
     "listen on an alternative port (default: 1982)",
     "port"},
    {"connect", 'c', POPT_ARG_STRING, &_connect_host, 'c',
     "join an existing replicant cluster through IP address or hostname",
     "addr"},
    {"connect-port", 'P', POPT_ARG_LONG, &_connect_port, 'C',
     "connect to an alternative port (default: 1982)",
     "port"},
    POPT_TABLEEND
};

} // extern "C"

int
main(int argc, const char* argv[])
{
    poptContext poptcon;
    poptcon = poptGetContext(NULL, argc, argv, popts, POPT_CONTEXT_POSIXMEHARDER);
    e::guard g = e::makeguard(poptFreeContext, poptcon); g.use_variable();
    int rc;

    while ((rc = poptGetNextOpt(poptcon)) != -1)
    {
        switch (rc)
        {
            case 'd':
                _daemonize = true;
                break;
            case 'f':
                _daemonize = false;
                break;
            case 'D':
                break;
            case 'l':
                try
                {
                    if (strcmp(_listen_host, "auto") == 0)
                    {
                        break;
                    }

                    _listen_ip = po6::net::ipaddr(_listen_host);
                }
                catch (po6::error& e)
                {
                    std::cerr << "cannot parse listen address" << std::endl;
                    return EXIT_FAILURE;
                }
                catch (std::invalid_argument& e)
                {
                    std::cerr << "cannot parse listen address" << std::endl;
                    return EXIT_FAILURE;
                }

                _listen = true;
                break;
            case 'L':
                if (_listen_port >= (1 << 16))
                {
                    std::cerr << "port number to listen on is out of range" << std::endl;
                    return EXIT_FAILURE;
                }

                _listen = true;
                break;
            case 'c':
                _connect = true;
                break;
            case 'C':
                if (_connect_port >= (1 << 16))
                {
                    std::cerr << "port number to connect to is out of range" << std::endl;
                    return EXIT_FAILURE;
                }

                _connect = true;
                break;
            case POPT_ERROR_NOARG:
            case POPT_ERROR_BADOPT:
            case POPT_ERROR_BADNUMBER:
            case POPT_ERROR_OVERFLOW:
                std::cerr << poptStrerror(rc) << " " << poptBadOption(poptcon, 0) << std::endl;
                return EXIT_FAILURE;
            case POPT_ERROR_OPTSTOODEEP:
            case POPT_ERROR_BADQUOTE:
            case POPT_ERROR_ERRNO:
            default:
                std::cerr << "logic error in argument parsing" << std::endl;
                return EXIT_FAILURE;
        }
    }

    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    try
    {
        replicant::daemon d;

        if (strcmp(_listen_host, "auto") == 0)
        {
            if (!busybee_discover(&_listen_ip))
            {
                std::cerr << "cannot automatically discover local address; specify one manually" << std::endl;
                return EXIT_FAILURE;
            }
        }

        po6::pathname data(_data);
        po6::net::location bind_to(_listen_ip, _listen_port);
        po6::net::hostname existing;

        if (_connect)
        {
            existing = po6::net::hostname(_connect_host, _connect_port);
        }

        return d.run(_daemonize, data, _listen, bind_to, _connect, existing);
    }
    catch (po6::error& e)
    {
        std::cerr << "system error:  " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
