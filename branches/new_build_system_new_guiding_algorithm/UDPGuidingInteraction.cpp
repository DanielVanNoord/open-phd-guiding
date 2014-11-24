//
//  matlab_interaction.cpp
//  PHD
//
//  Created by Stephan Wenninger
//  Copyright 2014, Max Planck Society.

/*
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
 *     Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */


#include "UDPGuidingInteraction.h"

#include "wx/string.h"
#include "wx/socket.h"


UDPGuidingInteraction::UDPGuidingInteraction(wxString host,
                                             wxString sendPort,
                                             wxString rcvPort)
: host(host),
  sendPort(sendPort),
  rcvPort(rcvPort),
  server(),
  sendClient(),
  receiveClient() {
    // Configure Sending
    sendClient.Hostname("localhost");     // configures client to local host...
    sendClient.Service(0);

    if (server.Hostname(host)) {             // validates destination host using DNS
        if (server.Service(sendPort)) {      // ensure port is valid
            sendSocket = new wxDatagramSocket(sendClient,wxSOCKET_NONE); // define the local port
            sendSocket->SetTimeout(1);
        }
    }

    // Configure Receiving
    receiveClient.AnyAddress();     // configures client to local host...
    receiveClient.Service(rcvPort);

    receiveSocket = new wxDatagramSocket(receiveClient,wxSOCKET_NONE); // define the local port
    receiveSocket->SetTimeout(2);
}

UDPGuidingInteraction::~UDPGuidingInteraction() {
}


bool UDPGuidingInteraction::SendToUDPPort(const void *buf, wxUint32 len) {
    while (!sendSocket->WaitForWrite()) {
    }
    sendSocket->SendTo(server, buf, len);
    return !sendSocket->Error();
}

bool UDPGuidingInteraction::ReceiveFromUDPPort(void * buf, wxUint32 len) {
    while (!receiveSocket->WaitForRead()) {
    }
    receiveSocket->RecvFrom(receiveClient, buf, len);
    return !receiveSocket->Error();
}
